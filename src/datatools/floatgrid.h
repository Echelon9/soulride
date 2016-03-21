// floatgrid.h		(c) Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic floating-point grid of data, w/ support for multiple
// layers.


#ifndef FLOATGRID_H
#define FLOATGRID_H


struct floatgrid
{
	floatgrid();	// makes an invalid grid; use set_size() to make valid.
	floatgrid(int layer_count, int xsize, int ysize);
	virtual ~floatgrid();

	void	set_size(int layer_count, int width, int height);

	int	get_layer_count() const { return m_layer_count; }
	int	get_width() const { return m_width; }
	int	get_height() const { return m_height; }

	// Element access.
	float	get(int layer, int x, int y) const;
	void	set(int layer, int x, int y, float value);

	// Row access.
	const float*	get_row(int layer, int y) const;
	float*	get_row(int layer, int y);

	// Whole-array access.
	const float*	get_layer(int layer) const;
	float*	get_layer(int layer);
	
	// Layer manipulation.
	void	add_layer();
	void	delete_layer(int layer);

private:
	int	m_layer_count, m_width, m_height;
	float**	m_layers;
};


#endif // FLOATGRID_H
