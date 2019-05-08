#!/usr/bin/perl
use strict;
use IO::CaptureOutput qw(capture qxx qxy);
#use Statistics::Descriptive;
use Cwd;
use File::Path;
use File::Copy;

my $nIterations = 1;

open( FILE, '>epcTimes.csv' );
print FILE "#sTime\tnodes\trTime\trTDev\n";
my $dir = getcwd;

my @nodes = (1, 10, 100);
my @interval = (200, 400, 800);


# Configure and complite first the program to avoid counting compilation time as running time
my $launch;
my $out, my $err;
my $path;
my @file = glob("*.txt");

foreach my $interval (@interval)
{
         foreach my $node (@nodes)
         {
            for ( my $iteration = 0 ; $iteration < $nIterations ; $iteration++ )
            {
               $launch = "NS_LOG=LteUeRrc:LteUeNetDevice:EpcEnbApplication ./waf --run 'lena-simple-epc --interPacketInterval=$interval --numberOfUeNodes=$node' > cwnd.txt 2>&1";
               #$launch = "NS_LOG=LteUeRrc:LteUeNetDevice:EpcEnbApplication ./waf --run 'lena-simple-epc --interPacketInterval=$interval --numberOfUeNodes=$node'";
               print "$launch\n";
               capture { system($launch ) } \$out, \$err;
               $err =~ /real(.+):(.+)/;
               $path = "results/$node $interval ms/";
               @file = glob("*.txt");
               if (! -d $path)
               {
                 my $dirs = eval { mkpath($path) };
                 die "Failed to create $path: $@\n" unless $dirs;
               }
               for my $file(@file)
               {
                  copy($file,$path) or die "Failed to copy $file: $!\n";
               }
               

            }
            print FILE "$interval\t$node\t";
         }
}
