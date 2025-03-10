require 'fileutils'

iters = ARGV[0]

[256, 512, 1024, 2048, 4096, 8192, 16384].each do |bins|
#  exe = "f:/xiasof~1/handel/benchmarks/xmap/mca_read.exe"
  exe = "f:/xiasof~1/handel/benchmarks/xmap/mca_read_module.exe"
  puts "Preparing to do #{bins} bins for #{iters} iterations."
  raise RuntimeError unless system(exe, iters, bins.to_s)
#  FileUtils.copy 'read_times.csv', "read_times_#{bins}bins_#{iters}.csv"
  FileUtils.copy('read_times_module.csv',
                 "read_times_module_#{bins}bins_#{iters}.csv")
end
