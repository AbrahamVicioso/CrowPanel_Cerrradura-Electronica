Import("env")
env.SetOption('num_jobs', 1)
print("Build parallelism limited to 1 (Windows MSYS2 compatibility)")
