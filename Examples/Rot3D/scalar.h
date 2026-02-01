#ifndef V3DLIB_ROT3D_SCALAR_H
#define V3DLIB_ROT3D_SCALAR_H

struct ScalarData {
	const int m_size;

	float *x = nullptr;
	float *y = nullptr;
	float *z = nullptr;

	ScalarData(int size);
	~ScalarData();

	void init();
	void disp(int show_number = -1);
};

void run_scalar_kernel();

#endif //  V3DLIB_ROT3D_SCALAR_H
