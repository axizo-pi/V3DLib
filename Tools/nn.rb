#!/usr/bin/env ruby
#
# This is a reference app for RNN's.
#
# Adapted from: https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/
#
##################################################################
require 'matrix'

##################################################################
# Utils 
##################################################################

$seed = 0
$s_m = 6012119

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


def frrand
	val = rrand
	return (-1.0 + 2.0*val/($s_m))
end


##################################################################
# Test methods
##################################################################

@primes = Array[ 
  2,	3,	5,	7,	11,	13,	17,	19,	23,	29,	31,	37,	41,	43,	47,	53,	59,	61,	67,	71,
 73, 79,	83,	89,	97,	101,	103,	107,	109,	113,	127,	131 #	137	139	149	151	157	163	167	173
]


def dump_matrix_header matrix, label
	"#{label}(#{matrix.row_count}, #{matrix.column_count})"
end


def dump_matrix matrix, label
	puts "#{dump_matrix_header(matrix, label)}:\n"

	if false
		buf = "0: "
		cur_row = 0 
		matrix.each_with_index do |e, row, col|
			if cur_row != row
				buf << "\n#{row}: "
				cur_row = row
			end

			buf << "#{e}, "
		end

		puts buf
	else
		buf = "#{label} = Array[\n"

		cur_row = 0 
		row_vec = [] 
		matrix.each_with_index do |e, row, col|
			if cur_row != row
				buf << "  [#{row_vec.join(", ")}],\n"
				row_vec = [] 

				cur_row = row
			end

			row_vec << e
		end

		buf << "  [#{row_vec.join(", ")}]\n"
		buf << "]"
		puts buf
	end
end

def init_matrix m, value
	r = 0
	while r < m.row_count do
		c = 0
		while c < m.column_count do
			m[r, c] = value 
			c += 1
		end
		r += 1
	end
end


def test_back_prop
	a = []
	(0...32).step(1) do |i|
		a << (1 + i)
	end

	puts "a1: #{a.join(", ")}"
	puts "d2: #{@primes.join(", ")}"

	#
	# gradient, outer product
	#
	d2 = Matrix.row_vector(@primes)
	a1 = Matrix.row_vector(a)

	w2_adj = a1.t * d2
	#dump_matrix w2_adj, "w2_adj"

	#
	# output layer to hidden layer
	# TODO: outer product here above, do rest
	#


	#
	# Hidden layer adjusting w1
	#
	puts "
##################################
# Test: Hidden layer adjusting w1
##################################"

=begin
	puts "### Using coords in back_prop ###"
	w2 = Matrix.zero(5, 3)
	init_matrix w2, 0.5
	dump_matrix w2, "w2"

	d2 = Matrix.zero(1, 3)
	init_matrix d2, 0.25
	dump_matrix d2, "d2"
=end	

	puts "### Using 16-blocks ###"
	w2 = Matrix.zero(32, 16)
	init_matrix w2, 0.5
	dump_matrix w2, "w2"

	d2 = Matrix.zero(1, 16)
	init_matrix d2, 0.25
	dump_matrix d2, "d2"

	#####
	tmp1 = (w2 * d2.t).t
	dump_matrix tmp1, "tmp1"

	a1 = Matrix.row_vector(a) # a[0...16])
	dump_matrix a1, "a1"

	#####
	tmp2 = a1.collect {|el| el*(1 - el) }    # sigmoid derivative
	puts dump_matrix tmp2, "tmp2" 

	d1   = tmp1.combine(tmp2) {|a, b| a*b}
	dump_matrix d1, "d1"

	puts "### Till Here ###"

	x = Matrix.zero(1, 16)
	init_matrix x, 1 

	#####
	w1_adj = x.t * d1  # gradient, outer product
	dump_matrix w1_adj, "w1_adj"
end


##################################################################
# Model
##################################################################

# initializing the weights randomly
def generate_wt(x, y)
	Matrix.build(x, y) {|row, col| rand }
end


class NeuralNetwork
	#
	# alpha:
	# 0.01 - doesn't work, always C
	# 0.1  - is OK, fails SOMETIMES
	# 1    - works great
	# 10   - doesn't work, always C
	#
	#
	def initialize hidden_size = 5, in_alpha = 1	
		@w1    = generate_wt(30, hidden_size)
		@w2    = generate_wt(hidden_size, 3)
		@bias1 = generate_wt(1, hidden_size)
		@bias2 = generate_wt(1, 3)

		@alpha = in_alpha
	end

	def w1; return @w1; end
	def w1= val; @w1 = val; end
	def w2; return @w2; end
	def w2= val; @w2 = val; end
	def a1; return @a1; end  					# result hidden layer, needed in back_prop
	def alpha; return @alpha; end
	def bias1; return @bias1; end
	def bias1= val; @bias1 = val; end
	def bias2; return @bias2; end
	def bias2= val; @bias2 = val; end

	# Creating the Feed forward neural network
	def f_forward(x)
		puts dump_matrix   x, "f_forward x" 
		puts dump_matrix @w1, "f_forward @w1" 

		# hidden
		z1 = x * @w1 + @bias1    # input from layer 1
		@a1 = sigmoid_m(z1)      # output of layer 2
		z2 = @a1 * @w2 + @bias2  # input of out layer
		a2 = sigmoid_m(z2)       # output of out layer

		a2
	end

end

def size m
	puts "(#{m.row(0).size}, #{m.row_count}))"
end


##################################################################

# A
a = Matrix[[ 
			0, 0, 1, 1, 0, 0,
   		0, 1, 0, 0, 1, 0,
   		1, 1, 1, 1, 1, 1,
   		1, 0, 0, 0, 0, 1,
   		1, 0, 0, 0, 0, 1
]]
# B
b = Matrix[[
			0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 1, 0,
   		0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 1, 0,
   		0, 1, 1, 1, 1, 0
		]]
# C
c = Matrix[[ 
			0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 1, 1, 1, 0
		]]

# Creating labels
y = Matrix[
			[1, 0, 0],
   		[0, 1, 0],
   		[0, 0, 1]
		]

x = [ a, b, c]

# activation function
def sigmoid x
	1.0/(1.0 + Math.exp(-x))
end

# Apply sigmoid to Matrix
def sigmoid_m z1
	z1.collect { |n| sigmoid(n) }
end


# for loss we will be using mean square error(MSE)
#
# out is Matrix of size nx1
# y is vector of length n
#
def loss(out, y)
	#p out
	#p y
	s = out.combine(y) {|a, b| (a - b)*(a - b) }

	#s = out.each_with_index do |e, row, col|
	#	(e - y[col])*(e - y[col])
	#end

	sum = 0
	s.each do |e|
		sum += e
	end

	sum/y.row(0).size
end


#
# Back propagation of error
#
# [input] - [w1, a1] - [w2, output]
#
# w2 <-- f(a1, d2)
# d1 <-- f(w2, d2, a1)
# w1 <-- f( x, d1)
#
def back_prop(x, y, nn)
	a2 = nn.f_forward x

	#
	# output layer to hidden layer
	#
	d2        = a2 - y                  # error in output layer
	puts "back_prop " + dump_matrix_header(d2, "d2")

	w2_adj    = nn.a1.t * d2            # gradient, outer product
	w2_tmp    = nn.w2 - nn.alpha*w2_adj
	nn.bias2 -= nn.alpha * d2

	#
	# Hidden layer adjusting w1
	#
	puts "
#######################################
# back_prop: Hidden layer adjusting w1
#######################################"

	tmp1 = (nn.w2 * d2.t).t
	#puts dump_matrix_header nn.w2, "w2" 
	puts dump_matrix nn.w2, "w2" 
	puts dump_matrix d2, "d2" 

	# Following also works, but has no multiline formatting
	#puts nn.w2

	#puts dump_matrix_header tmp1, "tmp1" 
	puts dump_matrix tmp1, "tmp1" 

	tmp2 = nn.a1.collect {|el| el*(1 - el) }    # sigmoid derivative
	d1   = tmp1.combine(tmp2) {|a, b| a*b}

	w1_adj = x.t * d1  # gradient, outer product
	puts dump_matrix w1_adj, "w1_adj" 

	nn.w1 -= nn.alpha*w1_adj
	nn.bias1 -= nn.alpha * d1

	nn.w2 = w2_tmp
end

###########################################

def train(x, y, nn, epoch = 10)
	acc =[]
	losss =[]

	(0...epoch).each { |j|
		l =[]

		x.each_with_index { |xi, i|
			out = nn.f_forward(xi)

			y_row = Matrix[y.row(i)]

			l.append loss(out, y_row)
			back_prop(xi, y_row, nn)
		}

		sum = 0
		l.each { |n|
			sum += n
		}

		avg_tmp = 1.0*sum/x.length

		if j % 10 == 0
			puts "epochs: #{j} ======== acc: #{(1 - avg_tmp)*100}"
		end

		acc.append (1 - avg_tmp)*100
		losss.append avg_tmp
	}

	return acc, losss
end


###########################################

def predict(x, nn)
	out = nn.f_forward(x)
	maxm = 0
	k = 0
	r = out.row(0)

	p out

	r.each_with_index { |n, i|
		if (maxm < n)
			maxm = n
			k = i
		end
	}

	if (k == 0)
		puts "Image is of letter A."
	elsif (k == 1)
		puts "Image is of letter B."
	else
		puts "Image is of letter C."
	end
end


####################
# Main
####################
nn = NeuralNetwork.new
puts dump_matrix_header nn.w2, "nn.w2" 

# To compare pseudo-random values with C++
#if true
#	(0...1000).step(1) do |n|
#		puts frrand.truncate(6)
#		#puts rrand
#	end
#
#	return
#end

if true
	test_back_prop
	return
end

# epoch:
# 100  - 99.98% acc 
# 1000 - 99.999% acc 
NumEpochs = 1  #1000
acc, losss = train(x, y, nn, NumEpochs)

# Example: Predicting for letter 'B'
predict(x[0], nn)
predict(x[1], nn)
predict(x[2], nn)
