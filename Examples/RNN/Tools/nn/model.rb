require_relative 'frrand'
require_relative 'settings'

##################################################################
# Support Methods
##################################################################

def size m
	puts "(#{m.row(0).size}, #{m.row_count}))"
end

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


##################################################################
# Model
##################################################################

# initializing the weights randomly
def generate_wt(x, y)
	Matrix.build(x, y) {|row, col| frrand }
end


#
# The original model is tiny:
#   - 30 input nodes
#   -  5 hidden nodes
#   -  3 output nodes
#
# This is smaller than the resolution of the QPU's,
# which deal with blocks of 16 values.
#
# Try values for alpha:
#   0.01 - doesn't work, always C
#   0.1  - is OK, fails SOMETIMES
#   1    - works great
#   10   - doesn't work, always C
#
class NeuralNetwork

	def initialize hidden_size = NUM_HIDDEN_NODES, in_alpha = 1	
		@w1    = generate_wt(NUM_INPUT_NODES, hidden_size)
		@w2    = generate_wt(hidden_size, NUM_OUTPUT_NODES)
		@bias1 = generate_wt(1, hidden_size)
		@bias2 = generate_wt(1, NUM_OUTPUT_NODES)
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

	#
	# Creating the Feed forward neural network
	#
	def f_forward(input)
		z1  = input * @w1         # Input from layer 1
		z1 += @bias1
		@a1 = sigmoid_m(z1)       # Output of layer 2
		z2  = @a1 * @w2
		z2 += @bias2
		a2 = sigmoid_m(z2)        # Output of out layer

		a2
	end

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
def back_prop(input, desired, nn)
	a2 = nn.f_forward input 

	#
	# output layer to hidden layer
	#
	d2        = a2 - desired            # error in output layer

	puts dump_matrix d2, "d2" 
	abort("Till Here");

	w2_adj    = nn.a1.t * d2            # gradient, outer product
	w2_tmp    = nn.w2 - nn.alpha*w2_adj
	nn.bias2 -= nn.alpha * d2

	#
	# Hidden layer adjusting w1
	#
=begin	
	puts "
#######################################
# back_prop: Hidden layer adjusting w1
#######################################"
=end

	tmp1 = (nn.w2 * d2.t).t
	#puts dump_matrix_header nn.w2, "w2" 
	#puts dump_matrix nn.w2, "w2" 
	#puts dump_matrix d2, "d2" 

	#puts dump_matrix_header tmp1, "tmp1" 
	#puts dump_matrix tmp1, "tmp1" 

	tmp2 = nn.a1.collect {|el| el*(1 - el) }    # sigmoid derivative
	d1   = tmp1.combine(tmp2) {|a, b| a*b}

	w1_adj = input.t * d1  # gradient, outer product
	#puts dump_matrix w1_adj, "w1_adj" 

	nn.w1 -= nn.alpha*w1_adj
	nn.bias1 -= nn.alpha * d1

	nn.w2 = w2_tmp
end

###########################################

def train(inputs, desireds, nn, epoch = 10)
	acc =[]
	losss =[]

	# Restrict epoch output somewhat
	skip_epoch = 1
	if epoch >= 1000
		skip_epoch = 50 
	elsif epoch >= 100
		skip_epoch = 10 
	end

	(0...epoch).each { |j|
		l =[]

		inputs.each_with_index { |input, i|
			out = nn.f_forward(input)

			desired = Matrix[desireds.row(i)]

			l.append loss(out, desired)
			back_prop(input, desired, nn)
		}

		sum = 0
		l.each { |n|
			sum += n
		}

		avg_tmp = 1.0*sum/inputs.length

		if j % skip_epoch == 0
			puts "epochs: #{j} ======== acc: #{(1 - avg_tmp)*100}"
		end

		acc.append (1 - avg_tmp)*100
		losss.append avg_tmp
	}

	return acc, losss
end


###########################################

def predict(input, nn)
	out = nn.f_forward(input)
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
