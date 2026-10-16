using GlassStats.Core;
using LibreHardwareMonitor.Hardware;

namespace GlassStats.Sensors;

public sealed class SensorReader : IDisposable
{
    private readonly Computer _computer;
    public SensorReader(bool cpuEnabled)
    {
        _computer = new() { IsCpuEnabled = cpuEnabled, IsGpuEnabled = true };
        _computer.Open();
    }
    public TemperatureFrame Read()
    {
        var sensors = new List<TemperatureSensor>();
        var errors = new List<string>();
        foreach (var hardware in _computer.Hardware)
        {
            try { Visit(hardware, sensors); }
            catch (Exception ex) { errors.Add(ex.GetType().Name + ": " + ex.Message); }
        }
        return new(1, DateTimeOffset.UtcNow, sensors.ToArray(), errors.Count > 0 ? string.Join("; ", errors) : null);
    }
    private static void Visit(IHardware hardware, List<TemperatureSensor> sensors)
    {
        hardware.Update();
        var kind = hardware.HardwareType == HardwareType.Cpu ? "CPU" : "GPU";
        foreach (var sensor in hardware.Sensors.Where(s => s.SensorType == SensorType.Temperature))
            sensors.Add(new(sensor.Identifier.ToString(), hardware.Name, sensor.Name, kind,
                sensor.Value is float value && float.IsFinite(value) ? value : null));
        foreach (var sub in hardware.SubHardware) Visit(sub, sensors);
    }
    public void Dispose() => _computer.Close();
}
