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

my @nodes = (200, 250, 300, 350, 400, 450, 500);
my @simDuration = (6, 12, 12, 12, 12, 32, 32);
my @trafficStartTime = (5.0, 10.0, 10.0, 10.0, 10.0, 30.0, 30.0);
#my @nodes = (100, 150,  300,  350,  400,  450,  540,  570,  600,  640,  670);
#my @time = (2,    3,    4,    5,    5,    5,    10,   10,   10,   10,   10);
#my @nodes = (  450,  540,  570,  600,  640,  670);
#my @time = (   50,    50,   50,   50,   50,   50);

# Configure and complite first the program to avoid counting compilation time as running time
my $launch;
my $i = 0;
my $j = 0;
my $out, my $err;
my $path;
my @file = glob("*.txt");


foreach my $node (@nodes)
{
   for ( my $iteration = 0 ; $iteration < $nIterations ; $iteration++ )
   {
      #$launch = "NS_LOG=LteUeRrc:LteUeNetDevice:EpcEnbApplication ./waf --run 'lena-simple-epc --simTime=$time --numberOfUeNodes=$node' > cwnd.txt 2>&1";
      $launch = "NS_LOG=LteUeNetDevice:LteEnbNetDevice:EpcEnbApplication:EpcUeNas:EpcSgwPgwApplication:PointToPointEpcHelper:RrFfMacScheduler ./waf --run 'lena-simple-epc --appTrafficStartTime=$trafficStartTime[$j++] --simTime=$simDuration[$i++] --numberOfUeNodes=$node' > cwnd.txt 2>&1";
      print "$launch\n";
      capture { system($launch ) } \$out, \$err;
      $err =~ /real(.+):(.+)/;
      $path = "results/$node/";
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
}

