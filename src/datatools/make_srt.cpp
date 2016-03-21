// make_srt.cpp	-- Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Program for converting some terrain data formats into a .srt file
// for use with the Soul Ride Engine (see http://soulride.com)


#include <stdio.h>
#include "heightfield.h"


void	print_usage()
{
	printf(
		"make_srt: a program to generate Soul Ride .srt data from USGS .BIL data.\n"
		"\n"
		"usage: make_srt <input_file> <output_file>\n"
		"\n"
		"     <input_file> should be the path and base name of at BIL dataset; for\n"
		"         for example, \"path/to/123456\", where the data is stored in\n"
		"         files such as path/to/123456.HDR, path/to/123456.BIL, etc\n"
		"\n"
		"     <output_file> is for the .srt file, loadable by Soul Ride or Shreditor\n");
}


int	main(int argc, const char* argv[])
{
	const char*	input_filename = NULL;
	const char*	output_filename = NULL;

	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Command-line switches.
			switch (argv[arg][1])
			{
			default:
				printf("Unknown command-line switch '%s'\n", argv[arg]);
				print_usage();
				exit(1);
			}
		}
		else if (input_filename == NULL)
		{
			input_filename = argv[arg];
		}
		else if (output_filename == NULL)
		{
			output_filename = argv[arg];
		}
		else
		{
			printf("extra junk '%s' on command line\n", argv[arg]);
			print_usage();
			exit(1);
		}
	}

	if (input_filename == NULL || output_filename == NULL)
	{
		printf("You must specify input and output\n");
		print_usage();
		exit(1);
	}

	// Pull in data file(s)
	heightfield	height_data;
	if (height_data.read_bil_as_height(input_filename) == false)
	{
		printf("Failed to read BIL input data.\n");
		exit(1);
	}

	// Open the output file.
	FILE*	out = fopen(output_filename, "rb");
	if (out == NULL)
	{
		printf("Can't open output file '%s'\n", output_filename);
		exit(1);
	}

	// Build quadtree data.
	corner_data	cd;
	cd.. = ..;
	minimal_qsquare*	tree_data = minimal_qsquare::build_subtree_if_necessary(
		height_data, cd, default_lod_spec());

	// Resample; write out data files.

	write_header(out);
	write_quadtree_height_data(out, &tree_data);
	write_surface_data(out);
	write_geometry(out);
	write_solids(out);
	write_objects(out);
	write_polygon_regions(out);

	// done.
	fclose(out);

	exit(0);
}


void	write_header(FILE* out)
// Write the beginning marker of an .srt file.
{
	char	buf[8] = {
		0xFF, 0xFF, 0xFF, 0xFF,		// this is a header marker (chosen via historical stupidity, sorry)
		12, 0, 0, 0			// version number, little-endian int
	}
	fwrite(buf, sizeof(buf), 1, fp);
}


struct minimal_qsquare
// Struct to hold the minimal data for a qsquare in an .srt file, plus
// links to child squares.  Build up a structure of these from the
// heightfield, so we can spew out the data easily.
{
	minimal_qsquare()
		:
		m_center(0),
		m_east(0),
		m_south(0)
	{
		memset(&m_child, sizeof(m_children), 0);
	}

	~minimal_qsquare()
	{
		for (int i = 0; i < 4; i++)
		{
			if (m_child[i])
			{
				delete m_child[i];
				m_child[i] = 0;
			}
		}
	}

	void	write(FILE* out) const
	{
		Write16(out, m_center);
		Write16(out, m_east);
		Write16(out, m_south);
		
		int	flags = 0;
		for (int i = 0; i < 4; i++)
		{
			if (m_child[i] != NULL)
			{
				flags |= (1 << i);
			}
		}

		fputc(flags, out);

		for (int i = 0; i < 4; i++)
		{
			if (m_child[i] != NULL)
			{
				m_child[i]->write(out);
			}
		}
	}


	static minimal_qsquare*	build_subtree_if_necessary(const heightfield* data, const corner_data& cd, const lod_specification* lod)
	// Given a heightfield, an lod_specification, and a specified
	// square of terrain, this function returns a minimal_qsquare
	// subtree if there's any data of interest inside the square.
	// If the terrain is basically uninteresting, then returns
	// NULL.
	{
		bool	has_data = false;

		minimal_qsquare	q = new minimal_qsquare;
		q->m_center = data->get_height(something);
		q->m_east = data->get_height(something);
		q->m_south = data->get_height(something);

		for (int i = 0; i < 4; i++)
		{
			corner_data	child_cd(cd, i);
			q->m_child[i] = build_subtree_if_necessary(data, child_cd, lod);
			if (q->m_child[i] != NULL)
			{
				has_data = true;
			}
		}

		if (has_data == false
		    && lod_specification->square_needs_data(cd))
		{
			has_data = true;
		}

		if (has_data == false)
		{
			// This square is useless, so don't return it.
			delete q;
			q = NULL;
		}

		return q;
	}

//data:
	unsigned short	m_center, m_east, m_south;
	minimal_qsquare*	m_child[4];
};


void	write_quadtree_height_data(FILE* out, const minimal_qsquare* tree)
// Write out height data in .srt quadtree format.
{
	assert(tree);
	tree->write(out);
}


void	write_surface_data(FILE* out)
// Surface types and lightmaps.  Dummy for now...
{
	...;
}


void	write_geometry(FILE* out)
// Geometry definitions.  Dummy for now...
{
}


void	write_solids(FILE* out)
// Collision solid definitions.  Dummy for now...
{
}


void	write_objects(FILE* out)
// Objects.
{
	// Dummy for now.  Maybe put a default spawn position in the
	// center of the terrain?
}


void	write_polygon_regions(FILE* out)
// Polygon (surface type, triggers?) regions.
{
	// Dummy for now.
}
