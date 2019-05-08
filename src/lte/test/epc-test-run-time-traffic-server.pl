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
# my @trafficStartTime = (2, 2, 2, 2, 2, 2, 2, 2, 3.0, 3.0, 6.0, 15.0);
# my @maxPacketsSet = (1);
# my @nodes = (10, 30, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500);




my @simDuration = (15);
my @trafficStartTime = (4);
my @maxPacketsSet = (2);
my @nodes = (2100, 2100, 8400, 8400);
my @distanceUe = (5, 10, 1, 2);
my @buildingWidth = (25, 50, 75, 100, 125, 150, 175, 200);



# my @simDuration = (3, 3, 5, 5, 8);
# my @trafficStartTime = (2, 2, 3.0, 3.0, 6.0);
# my @maxPacketsSet = (1);
# my @nodes = (250, 300, 350, 400, 450);

# Configure and complite first the program to avoid counting compilation time as running time
my $launch;
my $i = 0;
my $j = 0;
my $out, my $err;
my $path;
my @file = glob("*.txt");

#foreach my $distance (@distanceUe)
#{
  #foreach my $node (@nodes)
  #{
     for ( my $iteration = 4 ; $iteration < $nIterations ; $iteration++ )
     {
        #$launch = "NS_LOG=LteUeRrc:LteUeNetDevice:EpcEnbApplication ./waf --run 'lena-simple-epc --simTime=$time --numberOfUeNodes=$node' > cwnd.txt 2>&1";
        #$launch = "NS_LOG=LteUeNetDevice:LteEnbNetDevice:EpcEnbApplication:EpcUeNas:EpcSgwPgwApplication:PointToPointEpcHelper:LteUeRrc ./waf --run 'lena-simple-epc --maxPackets=2 --numberOfUeNodes=$nodes[$j] --appTrafficStartTime=4 --simTime=15 --ueDistance=$distanceUe[$j]' > cwnd.txt 2>&1";
        $launch = "NS_LOG=LteUeNetDevice:LteEnbNetDevice:EpcEnbApplication:EpcUeNas:EpcSgwPgwApplication:PointToPointEpcHelper:LteUeRrc ./waf --run 'lena-simple-epc --maxPackets=2 --numberOfUeNodes=300 --appTrafficStartTime=2 --simTime=20 --ueDistance=2.21 --buildingWidth=$buildingWidth[$iteration]'> cwnd.txt 2>&1";
        print "$launch\n";
        capture { system($launch ) } \$out, \$err;
        $err =~ /real(.+):(.+)/;
        $path = "results/D$buildingWidth[$iteration] N$nodes[$iteration]";
        $j += 1;
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
  #}
#}

