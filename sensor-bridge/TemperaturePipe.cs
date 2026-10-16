using System.IO.Pipes;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Text.Json;
using GlassStats.Core;

namespace GlassStats.Windows;

public sealed class TemperatureClient : IDisposable
{
    private readonly CancellationTokenSource _stop = new();
    private TemperatureFrame? _latest;
    public TemperatureFrame? Latest => Volatile.Read(ref _latest);
    private readonly string _pipePrefix;
    public TemperatureClient(string pipePrefix = "GlassStatsQt.Temperatures.") { _pipePrefix = pipePrefix; _ = RunAsync(_stop.Token); }
    private async Task RunAsync(CancellationToken ct)
    {
        var sid = WindowsIdentity.GetCurrent().User!.Value;
        while (!ct.IsCancellationRequested)
        {
            try
            {
                using var pipe = new NamedPipeClientStream(".", _pipePrefix + sid, PipeDirection.In, PipeOptions.Asynchronous);
                await pipe.ConnectAsync(1500, ct);
                using var reader = new StreamReader(pipe);
                while (!ct.IsCancellationRequested)
                {
                    // A timeout closes the pipe and permits recovery from a hung helper.
                    var line = await reader.ReadLineAsync(ct).AsTask().WaitAsync(TimeSpan.FromSeconds(5), ct);
                    if (line == null || line.Length > 1024 * 1024) break;
                    var frame = JsonSerializer.Deserialize<TemperatureFrame>(line);
                    if (frame?.Version == 1) Volatile.Write(ref _latest, frame);
                }
            }
            catch (Exception ex) when (ex is IOException or TimeoutException or JsonException or OperationCanceledException or UnauthorizedAccessException) { }
            try { await Task.Delay(2000, ct); } catch (OperationCanceledException) { break; }
        }
    }
    public void Dispose() => _stop.Cancel();
}

public static class TemperaturePipe
{
    public static NamedPipeServerStream Create(SecurityIdentifier owner, string prefix = "GlassStatsQt.Temperatures.")
    {
        var security = new PipeSecurity();
        security.SetAccessRuleProtection(true, false);
        security.AddAccessRule(new(new SecurityIdentifier(WellKnownSidType.NetworkSid, null), PipeAccessRights.FullControl, AccessControlType.Deny));
        security.AddAccessRule(new(owner, PipeAccessRights.Read | PipeAccessRights.Synchronize, AccessControlType.Allow));
        security.AddAccessRule(new(new SecurityIdentifier(WellKnownSidType.LocalSystemSid, null), PipeAccessRights.FullControl, AccessControlType.Allow));
        return NamedPipeServerStreamAcl.Create(prefix + owner.Value, PipeDirection.Out, 1,
            PipeTransmissionMode.Byte, PipeOptions.Asynchronous, 0, 65536, security);
    }
}
