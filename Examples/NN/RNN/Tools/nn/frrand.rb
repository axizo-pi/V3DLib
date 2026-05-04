
$seed = 0
$s_m = 6012119
$s_frrand_count = 0

#
# Sample random numbers using a linear congruential generator.
#
# The goal is have exactly the same random number generator on ruby and C++.
# Checked for the first million values.
#
# Source: https://predictivesciencelab.github.io/data-analytics-se/lecture07/hands-on-07.1.html
#
def lcg x
  a = 123456
  b = 978564

	((((a * x) % 0x100000000) + b) % 0x100000000) % ($s_m)
end		

def rrand
	$seed = lcg($seed)
end


#
# Return a pseudo-random float value between -1 and 1.
#
def frrand
	$s_frrand_count += 1
	val = rrand
	return (-1.0 + 2.0*val/($s_m))
end


def frrand_count
	$s_frrand_count
end


#
# Debug method to check output and to compare it to the c++ version of frrand().
#
def disp_random
	buf= "frrand() output:\n"

	(0...1000).step(1) do |n|
		buf += frrand.truncate(6).to_s + ", "

		if (n % 32 == 0) 
			buf += "\n"
		end
	end

	puts "#{buf}\n"
end
