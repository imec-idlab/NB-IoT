#!/usr/bin/perl
use strict;
use IO::CaptureOutput qw(capture qxx qxy);
#use Statistics::Descriptive;
use Cwd;
use File::Path;
use File::Copy;

my $nIterations = 8;

open( FILE, '>epcTimes.csv' );
print FILE "#sTime\tnodes\trTime\trTDev\n";
my $dir = getcwd;

# my @simDuration = (3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 8, 17);
# my @appStartTime = (2, 2, 2, 2, 2, 2, 2, 2, 3.0, 3.0, 6.0, 15.0);
# my @maxPacketsSet = (1);
# my @nodes = (10, 30, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500);




#my @simDuration = (30, 30, 30, 30, 65, 90);
my @appStartTime = (5, 5, 5, 6, 20, 100, 200);
my @maxPacketsSet = (2);
my @nodes = (100, 200, 300, 400, 500, 600);
my @distanceUe = (6.63, 3.315, 2.21, 1.65, 1.32, 1.1);
my @buildingWidth = (0, 25, 50, 75, 100, 125, 150, 175, 200);
my @maxPacketInterval = (20000000, 20000000, 20000000, 20000000, 20000000, 40000000);



# my @simDuration = (3, 3, 5, 5, 8);
# my @appStartTime = (2, 2, 3.0, 3.0, 6.0);
# my @maxPacketsSet = (1);
# my @nodes = (250, 300, 350, 400, 450);

# Configure and complite first the program to avoid counting compilation time as running time
my $launch;
my $i = 0;
my $j = 0;
my $out, my $err;
my $path;
my $simDurationAgg = 0;
my @file = glob("*.txt");

#foreach my $distance (@distanceUe)
#{
  #foreach my $node (@nodes)
  #{
    for ( my $scaleCounter = 0 ; $scaleCounter < 6 ; $scaleCounter++)
    {
      for ( my $widthCounter = 0 ; $widthCounter < 1 ; $widthCounter++ )
        {
          $simDurationAgg = $appStartTime[$scaleCounter] + $maxPacketInterval[$scaleCounter]/1000000 + 5;
          $launch = "NS_LOG=LteUeNetDevice:LteEnbNetDevice:EpcEnbApplication:EpcUeNas:EpcSgwPgwApplication:PointToPointEpcHelper:LteUeRrc ./waf --run 'lena-simple-epc --maxPackets=2 --numberOfUeNodes=$nodes[$scaleCounter] --appTrafficStartTime=$appStartTime[$scaleCounter] --interPacketInterval=$maxPacketInterval[$scaleCounter] --simTime=$simDurationAgg --ueDistance=$distanceUe[$scaleCounter] --buildingWidth=$buildingWidth[$widthCounter]'> cwnd.txt 2>&1";
          print "$launch\n";
          capture { system($launch ) } \$out, \$err;
          $err =~ /real(.+):(.+)/;
          $path = "results/W$buildingWidth[$widthCounter] N$nodes[$scaleCounter]";
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
  #}
#}

