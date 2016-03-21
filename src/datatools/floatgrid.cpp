// floatgrid.cpp		(c) Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic floating-point grid of data, w/ support for multiple
// layers.


#include "floatgrid.h"
#include <assert.h>


floatgrid::floatgrid()
	:
	m_layer_count(0),
	m_width(0),
	m_height(0)
{
}


floatgrid::floatgrid(int layer_count, int xsize, int ysize)
	:
	m_layer_count(0),
	m_width(0),
	m_height(0)
// Constructor.
{
	set_size(layer_count, xsize, ysize);
}


void	floatgrid::set_size(int layer_count, int xsize, int ysize)
{
	assert(m_layer_count == 0);
	assert(m_width == 0);
	assert(m_height == 0);

	m_layer_count = layer_count;
	m_width(xsize);
	m_height(ysize);

	assert(layer_count >= 0);
	assert(xsize >= 0);
	assert(ysize >= 0);

	int	layer_size = m_width * m_height;

	// Create the layer index.
	if (m_layer_count > 0)
	{
		m_layers = new float*[m_layer_count];
	}
	else
	{
		m_layers = 0;
	}

	// Create the actual layer data.
	for (int i = 0; i < m_layer_count; i++)
	{
		m_layers[i] = new float[layer_size];
	}

	// @@ fill with zeros?
}


floatgrid::~floatgrid()
// Destructor, deallocate our stuff.
{
	for (int i = 0; i < m_layer_count; i++)
	{
		delete [] m_layers[i];
	}

	delete [] m_layers;
}


float	get(int layer, int x, int y) const
// Return the value at the given coords.
{
	assert(layer >= 0 && layer < m_layer_count);
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);

	return *(m_layers[layer] + y * m_width + x);
}


void	set(int layer, int x, int y, float value)
// Insert the given value at the given coords.
{
	assert(layer >= 0 && layer < m_layer_count);
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);

	*(m_layers[layer] + y * m_width + x) = value;
}


const float*	get_row(int layer, int y) const
// Return a pointer to the start of the specified row of data.
{
	assert(layer >= 0 && layer < m_layer_count);
	assert(y >= 0 && y < m_height);
	
	return m_layers[layer] + y * m_width;
}


float*	get_row(int layer, int y)
// non-const version of above.
{
	assert(layer >= 0 && layer < m_layer_count);
	assert(y >= 0 && y < m_height);
	
	return m_layers[layer] + y * m_width;
}


const float*	get_layer(int layer) const
// Return a pointer to the whole array of floats for the specified
// layer.
{
	assert(layer >= 0 && layer < m_layer_count);

	return m_layers[layer];
}


float*	get_layer(int layer)
// non-const version of above.
{
	assert(layer >= 0 && layer < m_layer_count);

	return m_layers[layer];
}
	

void	add_layer()
// Add another layer to our set.  (New layer gets added at the end.)
{
	int	new_layer_count = m_layer_count + 1;

	assert(new_layer_count > 0);

	// Create a new layer index.
	float**	new_layers = new float*[new_layer_count];

	// Copy existing pointers.
	for (int i = 0; i < m_layer_count; i++)
	{
		new_layers[i] = m_layers[i];
	}

	// Allocate the new layer.
	new_layers[new_layer_count - 1] = new float[m_width * m_height];

	// Swap indexes.
	delete [] m_layers;
	m_layers = new_layers;
}


void	delete_layer(int layer)
// Remove the specified layer.
{
	assert(layer >= 0 && layer < m_layer_count);
	assert(m_layer_count > 0);

	m_layer_count -= 1;

	delete [] m_layers[layer];

	// Shift higher layers down.
	for (int i = layer; i < m_layer_count; i++)
	{
		m_layers[i] = m_layers[i + 1];
	}

	// @@ don't strictly need to reallocate m_layers[].  Leave it be
	// for now...
}
