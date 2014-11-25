#!/usr/bin/perl

use strict;
use warnings;

my $KLEE = "klee --write-pcs --libc=uclibc --posix-runtime";
my $KLEAVER = "kleaver";
my $LLI = "lli";
my $BIPATH_HELPER = "bipath";
my $debug = 1;

sub main();
sub execute($$@);
sub bipath_is_cached($@);

main();

sub main() {
    my $mode = shift @ARGV;
    my $program = shift @ARGV;
    my @args = @ARGV;

    if($mode eq '--force-native') {
        execute(1, $program, @args);
    }
    elsif($mode eq '--force-klee') {
        execute(0, $program, @args);
    }
    elsif($mode eq '--bipath') {
        if(bipath_is_cached($program, @args)) {
            execute(1, $program, @args);
        }
        else {
            execute(0, $program, @args);
        }
    }
    else {
        print STDERR <<EOF;
Usage: $0 (--force-native|--force-klee|--bipath) program [args...]

Runs the given program with both native and klee support. When run under
klee, the supplied arguments will be replaced with symbolic arguments
and the test cases cached. Later invocations with similar-length arguments
can then be run natively.
EOF
        exit 1;
    }
}

sub execute($$@) {
    my $native = shift @_;
    my $program = shift @_;
    my @args = @_;
    
    my $status = 0;
    if($native) {
        $status = system("$LLI $program @args") >> 8;
    }
    else {
        $status = system("$KLEE $program @args") >> 8;
    }
    print "Exit code: $status\n" if $debug;
}

sub bipath_is_cached($@) {
    my $program = shift @_;
    my @args = @_;

    # slurp in all test data from previous symbolic runs
    for my $dir (glob "klee-out-*") {
        print "processing directory $dir\n" if $debug;
        my $TEMP_DIR = "/tmp/bipath-temp";

        # helper: convert query form
        system("$BIPATH_HELPER $dir $TEMP_DIR");
        
        for my $file (glob "$TEMP_DIR/*.pc") {
            print "    processing file $file\n" if $debug;
            my @arrays = ();

            # look for lines which define arrays
            open(FILE, $file) or die;
            while(my $line = <FILE>) {
                chomp $line;
                if($line =~ /^array\s+([\w\d_]+)\[(\d+)\]\s+:([^=]+)=\s+(.*?)$/) {
                    push @arrays, {
                        'is_array' => 1,
                        'name' => $1,
                        'size' => int $2,
                        'type' => $3,
                        'init' => $4,
                        'raw' => $line
                    };
                    print "        found array $arrays[$#arrays]->{'name'}\n"
                        if $debug;
                }
                else {
                    push @arrays, {
                        'is_array' => 0,
                        'raw' => $line
                    };
                }
            }

            close FILE;

            # hard-coded: replace model_version with 1
            for my $array (@arrays) {
                if($array->{'is_array'} && $array->{'name'} eq 'model_version'
                    && $array->{'init'} eq 'symbolic') {

                    $array->{'init'} = "[1 0 0 0]";  # little endian
                }
            }

            # see if this test case is suitable (strings in @args exist as
            # symbolic arrays, and their initializers aren't too long)
            my $suitable = 1;
            for my $argc (0..$#args) {
                my $arg = $args[$argc];
                my $found = 0;
                for my $array (@arrays) {
                    if($array->{'is_array'} && $array->{'name'} eq 'arg'.$argc) {
                        print "        matching arg $argc... ".$array."\n";
                        #print length($arg), "==", $array->{'size'}, "\n";
                        #print "[$array->{'init'}]\n";
                        if(length($arg) + 1 <= $array->{'size'}
                            && $array->{'init'} eq 'symbolic') {

                            my @numbers = map(ord, split(//, $arg));
                            push @numbers, 0;  # add null terminator
                            $array->{'init'} = "[" . join(' ', @numbers) . "]";
                            $found = 1;
                        }
                    }
                }

                $suitable = 0 if !$found;
                last if !$suitable;
            }

            # if still suitable, write temp file and call kleaver
            if($suitable) {
                open(TEMP, ">$file.temp") or die;
                for my $array (@arrays) {
                    if($array->{'is_array'}) {
                        print TEMP "array $array->{'name'}"
                            . '[' . $array->{'size'} . ']'
                            . " : $array->{'type'} = $array->{'init'}\n";
                    }
                    else {
                        print TEMP "$array->{'raw'}\n";
                    }
                }
                close TEMP;

                print "        invoke kleaver on $file.temp\n" if $debug;

                open(KLEAVER, "$KLEAVER $file.temp|") or die;
                while(my $response = <KLEAVER>) {
                    if($response =~ /Query 0:\s+(\w+)/) {
                        print "            kleaver says '$1'\n";
                        if($1 eq 'VALID') {
                            print "*** RUNNING NATIVE\n" if $debug;
                            return 1;   # success! satisfies the constraints
                        }
                    }
                }
                close KLEAVER;
            }
        }
    }

    print "*** RUNNING IN KLEE\n" if $debug;
    return 0;
}
