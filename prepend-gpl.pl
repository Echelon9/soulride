#!/usr/bin/perl


for my $f (@ARGV)
{
    `cat pre.txt $f > $f.tmp`;
    `mv $f.tmp $f`;
}
