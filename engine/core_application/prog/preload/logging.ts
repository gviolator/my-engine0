
enum LogLevel
{
    Verbose = 1 << 1,
    Debug = 1 << 2,
    Info = 1 << 3,
    Warning = 1 << 4,
    Error = 1 << 5,
    Critical = 1 << 6
}

function mylog(this: void, message: string): void 
{
    mylog_native((LogLevel.Verbose as number), message);
}

function mylog_verbose(this: void, message: string): void
{
    mylog_native(LogLevel.Verbose, message);
}

function mylog_debug(this: void, message: string): void
{
    mylog_native(LogLevel.Debug, message);
}