using System.Diagnostics;
using System.IO.Pipes;
using System.Security.AccessControl;
using System.Security.Principal;
using System.ServiceProcess;
using System.Text;
using System.Text.Json;
using GlassStats.Sensors;
using GlassStats.Windows;

namespace GlassStats.SensorsBridge;

internal static class Program
{
    internal const string ServiceName = "GlassStatsQtSensors";
    internal static string ConfigPath => Path.Combine(AppContext.BaseDirectory, "owner-sid.txt");
    private static int Main(string[] args)
    {
        try
        {
            if (args.FirstOrDefault() == "--worker")
            {
                using var sensorReader = new SensorReader(false);
                while (true)
                {
                    Console.WriteLine(JsonSerializer.Serialize(sensorReader.Read()));
                    Thread.Sleep(1000);
                }
            }
            if (args.FirstOrDefault() == "--install" && args.Length == 2)
            {
                var sid = new SecurityIdentifier(args[1]);
                if (!sid.IsAccountSid()) throw new ArgumentException("An account SID is required.");
                var path = Environment.ProcessPath!;
                var expected = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "GlassStatsQtHelper", "GlassStats.SensorsBridge.exe");
                if (!Path.GetFullPath(path).Equals(expected, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException("Install the helper in Program Files before registering it.");
                File.WriteAllText(ConfigPath, sid.Value);
                if (ServiceController.GetServices().Any(s => s.ServiceName == ServiceName))
                {
                    Sc("stop", ServiceName, allowFailure: true);
                    Sc("config", ServiceName, "binPath=", "\"" + path + "\"", "start=", "auto");
                }
                else Sc("create", ServiceName, "binPath=", "\"" + path + "\"", "start=", "auto", "DisplayName=", "GlassStats temperature sensors");
                Sc("failure", ServiceName, "reset=", "86400", "actions=", "restart/5000/restart/15000/restart/60000");
                Sc("start", ServiceName);
                return 0;
            }
            if (args.FirstOrDefault() == "--uninstall")
            {
                Sc("stop", ServiceName, allowFailure: true);
                Sc("delete", ServiceName, allowFailure: true);
                if (File.Exists(ConfigPath)) File.Delete(ConfigPath);
                return 0;
            }
            if (args.FirstOrDefault() == "--probe")
            {
                using var sensors = new SensorReader(true);
                Console.WriteLine(JsonSerializer.Serialize(sensors.Read())); return 0;
            }
            ServiceBase.Run(new SensorService()); return 0;
        }
        catch (Exception ex) { Console.Error.WriteLine(ex); return 1; }
    }
    private static void Sc(string first, string second, bool allowFailure = false) => Sc([first, second], allowFailure);
    private static void Sc(params string[] args) => Sc(args, false);
    private static void Sc(string[] args, bool allowFailure)
    {
        var info = new ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "sc.exe")) { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
        foreach (var arg in args) info.ArgumentList.Add(arg);
        using var process = Process.Start(info)!;
        var output = process.StandardOutput.ReadToEnd(); var error = process.StandardError.ReadToEnd();
        process.WaitForExit();
        if (process.ExitCode != 0 && !allowFailure) throw new InvalidOperationException(output + error);
    }
}

internal sealed class SensorService : ServiceBase
{
    private readonly CancellationTokenSource _stop = new();
    private Task? _task;
    public SensorService() { ServiceName = Program.ServiceName; }
    protected override void OnStart(string[] args)
    {
        var owner = new SecurityIdentifier(File.ReadAllText(Program.ConfigPath).Trim());
        _task = Task.Run(() => ServeAsync(owner, _stop.Token));
    }
    protected override void OnStop() { _stop.Cancel(); _task?.Wait(TimeSpan.FromSeconds(4)); }
    private static async Task ServeAsync(SecurityIdentifier owner, CancellationToken ct)
    {
        while (!ct.IsCancellationRequested)
        {
            try
            {
                using var pipe = TemperaturePipe.Create(owner);
                await pipe.WaitForConnectionAsync(ct);
                // Open hardware only while a client is connected. Never expose hardware control operations.
                using var sensors = new SensorReader(true);
                using var writer = new StreamWriter(pipe, new UTF8Encoding(false)) { AutoFlush = true };
                while (pipe.IsConnected && !ct.IsCancellationRequested)
                {
                    var json = JsonSerializer.Serialize(sensors.Read());
                    await writer.WriteLineAsync(json.AsMemory(), ct);
                    await Task.Delay(1000, ct);
                }
            }
            catch (OperationCanceledException) { break; }
            catch (Exception) { try { await Task.Delay(2000, ct); } catch (OperationCanceledException) { break; } }
        }
    }
}
