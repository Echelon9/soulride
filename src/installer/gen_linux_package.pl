#!perl

#    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.
#
#    This file is part of The Soul Ride Engine, see http://soulride.com
#
#    The Soul Ride Engine is free software; you can redistribute it
#    and/or modify it under the terms of the GNU General Public License
#    as published by the Free Software Foundation; either version 2 of
#    the License, or (at your option) any later version.
#
#    The Soul Ride Engine is distributed in the hope that it will be
#    useful, but WITHOUT ANY WARRANTY; without even the implied
#    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Foobar; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Script to pack up a tarball for the Linux version of Soul Ride.

# First arg is the output filename, e.g. "virtual_stratton_linux_1_0c.exe"
# Second arg is the filename containing a list of files to go in the package.

use strict;


my ($outfile, $mountain_name, $filelist) = @ARGV;

if ($outfile eq ''
    || $filelist eq '')
{
	print "Usage: gen_linux_package.pl <outfile> <filelist>\n";
	exit(1);
}

my $basepath = "../../../";
my $returnpath = "soulride/src/installer/";
my $prefix = "soulride/";


# Get the list of files to include, and massage them into the format that
# tar wants.
my $install_files;
my @files;
my %file_hash;
my $current_path = '---';
open IN, $filelist;
while (<IN>) {
	chomp($_);
	my $file = $_;

	if ($file ne '' && $file_hash{$file} != 1) {
		$file_hash{$file} = 1;  # Avoid installing duplicates.

		my $real_filename = get_actual_filename($prefix . $file, $basepath);

		$install_files = $install_files . $real_filename . "\n";
	}
}
close IN;


# Include a shortcut for this mountain.
$install_files = $install_files . "soulride/start_${mountain_name}.sh\n";
$install_files = $install_files . "soulride/start_${mountain_name}_static.sh\n";

# Include a linux readme.
$install_files = $install_files . "soulride/readme-linux.txt\n";


$install_files =~ s/.exe//g;  # Replace .exe with nothing (our filelist is designed for Windows)

# Include the static executable.
$install_files = $install_files . "soulride/soulride-static\n";


open(OUT, ">tmp.lst") or die "can't open tmp.lst: $!";
print OUT $install_files;
close(OUT);


`cd $basepath; tar czvf $returnpath$outfile --files-from ${returnpath}tmp.lst`;


# print(get_actual_filename("soulride/data/Breckenridge/whalestail.srm", $basepath));


# Function to extract the filename with the capitalization as it
# actually exists on disk (Linux file systems are usually case-sensitive, 
# whereas Win32 ones aren't).
sub get_actual_filename
{
    my ($filename, $basepath) = @_;

    $filename =~ /^(.*\/)([^\/]+)$/;
    my ($path, $file) = ($1, $2);

    opendir(DIR, $basepath . $path) or die "can't open dir $basepath$path: $!";
    my @filelist = readdir(DIR);
    close(DIR);

    # Look for a match.
    map {
	if (lc($_) eq lc($file)) { $file = $_; }
    } @filelist;

    return $path . $file;
}
