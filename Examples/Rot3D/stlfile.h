#ifndef V3DLIB_ROT3D_STLFILE_H
#define V3DLIB_ROT3D_STLFILE_H
#include <string>

struct StlData {
	StlData(int size);

	virtual float xc(int index) const = 0;
	virtual float yc(int index) const = 0;
	virtual float zc(int index) const = 0;

	virtual void xc(int index, float rhs) = 0;
	virtual void yc(int index, float rhs) = 0;
	virtual void zc(int index, float rhs) = 0;

	int size() const { return m_size; }
	void init();
	void disp(std::string const &label = "", int show_number = -1) const;
	void init_v(int index, float in_x, float in_y, float in_z);

private:	
	int m_size;

	void disp_arrays(int size) const;
};


void load_stl(std::string const &filename);

int stl_num_coords();
void stl_load_data(StlData &data);
void stl_save_data(StlData &data, bool save_file = false);

#endif // V3DLIB_ROT3D_STLFILE_H
