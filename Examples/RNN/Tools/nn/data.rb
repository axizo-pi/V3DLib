require_relative "settings"

##################################################################
# Input data and desired outputs
##################################################################

# Letter A
a = [ 
			0, 0, 1, 1, 0, 0,
   		0, 1, 0, 0, 1, 0,
   		1, 1, 1, 1, 1, 1,
   		1, 0, 0, 0, 0, 1,
   		1, 0, 0, 0, 0, 1
]

# Letter B
b = [
			0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 1, 0,
   		0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 1, 0,
   		0, 1, 1, 1, 1, 0
]

# Letter C
c = [ 
			0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 1, 1, 1, 0
]


#
# Labels - contrived plural to indicate that there are multiple values
#
label_a =	[1, 0, 0]
label_b =	[0, 1, 0]
label_c =	[0, 0, 1]



#
# Adjust input and desired value to the dimensions of the model
#
raise "Inputs larger than input size of model " if a.size > NUM_INPUT_NODES

if a.size < NUM_INPUT_NODES
	diff = NUM_INPUT_NODES - a.size
	v_add =  Vector.zero(diff)

	a +=  v_add.to_a
	b +=  v_add.to_a
	c +=  v_add.to_a
end

raise "Labels larger than output size of model " if label_a.size > NUM_OUTPUT_NODES

if label_a.size < NUM_OUTPUT_NODES
	diff = NUM_OUTPUT_NODES - label_a.size
	v_add =  Vector.zero(diff)

	label_a +=  v_add.to_a
	label_b +=  v_add.to_a
	label_c +=  v_add.to_a
end


$inputs = [
	Matrix[a],
	Matrix[b],
	Matrix[c]
]

$desireds = Matrix[
	label_a,
	label_b,
	label_c
]
