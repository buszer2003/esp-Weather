Import("env")

env.Replace(PROGNAME="fw_esp-weather_v%s" % env.GetProjectOption("custom_prog_version"))