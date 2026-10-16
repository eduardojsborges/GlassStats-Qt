namespace GlassStats.Core;
public sealed record TemperatureSensor(string Id, string DeviceName, string Name, string Kind, double? Celsius);
public sealed record TemperatureFrame(int Version, DateTimeOffset Timestamp, TemperatureSensor[] Sensors, string? Error = null);
