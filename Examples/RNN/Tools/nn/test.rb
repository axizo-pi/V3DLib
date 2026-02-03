
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
