#!/usr/bin/perl

use strict;
use warnings;

use Time::HiRes qw/ time /;

my $KLEE = "klee --write-pcs --libc=uclibc --posix-runtime";
my $KLEAVER = "kleaver";
my $LLI = "lli";
my $BIPATH_HELPER = "bipath";
my $debug = 1;
my $with_time = 1;

sub main();
sub write_arg_counts(@);
sub execute($$@);
sub bipath_is_cached($@);
sub promote_args(@);

main();

sub main() {
    my $mode = 'bipath';

    if(scalar(@ARGV) < 1 || $ARGV[0] eq '--help') {
        print STDERR <<EOF;
Usage: $0 [--force-native|--force-klee|--bipath] program [args...]

Runs the given program with both native and klee support. When run under
klee, the supplied arguments will be replaced with symbolic arguments
and the test cases cached. Later invocations with similar-length arguments
can then be run natively.
EOF
        exit 1;
    }

    elsif($ARGV[0] eq '--force-native') {
        shift @ARGV;
        $mode = 'native';
    }
    elsif($ARGV[0] eq '--force-klee') {
        shift @ARGV;
        $mode = 'klee';
    }
    elsif($ARGV[0] eq '--bipath') {
        shift @ARGV;
        $mode = 'bipath';
    }

    my $program = shift @ARGV;
    my @args = @ARGV;
    my $did_klee = 0;

    if($mode eq 'native') {
        execute(1, $program, @args);
    }
    elsif($mode eq 'klee') {
        execute(0, $program, @args);
        write_arg_counts(@args);
    }
    else {
        if(!bipath_is_cached($program, @args)) {
            my @new_args = promote_args(@args);
            execute(0, $program, @new_args);
            write_arg_counts(@args);

            if(bipath_is_cached($program, @args)) {
                execute(1, $program, @args);
            }
        }
        else {
            # we can already run natively, just do it
            execute(1, $program, @args);
        }
    }
}

sub write_arg_counts(@) {
    my @args = @_;
    # hack: remember how many arguments we had
    open(COUNT, ">klee-last/arg_count") or die;
    for my $a (@args) {
        print COUNT length($a), "\n";
    }
    close COUNT;
}

sub execute($$@) {
    my $native = shift @_;
    my $program = shift @_;
    my @args = @_;
    
    my $status = 0;
    if($native) {
        $program =~ /(.*)\.bc/;
        my $binary = $1;
        if(-x $binary) {
            if($binary !~ m|/|) {
                $binary = "./$binary";
            }
            print "exec: `$binary @args`\n" if $debug;

            my $native_start_time = time;
            $status = system("$binary @args") >> 8;
            my $native_time = time - $native_start_time;

            print "BipathTime(native): $native_time\n" if $with_time;
        }
        else {
            print "exec: `$LLI $program @args`\n" if $debug;
            my $lli_start_time = time;
            $status = system("$LLI $program @args") >> 8;
            my $lli_time = time - $lli_start_time;
            print "BipathTime(lli): $lli_time\n" if $with_time;
        }
    }
    else {
        print "exec: `$KLEE $program @args`\n" if $debug;

        my $klee_start_time = time;
        $status = system("$KLEE $program @args") >> 8;
        my $klee_time = time - $klee_start_time;
        print "BipathTime(klee): $klee_time\n" if $with_time;
    }
    print "Exit code: $status\n" if $debug;
}

sub bipath_is_cached($@) {
    my $program = shift @_;
    my @args = @_;

    my $kleaver_start_time = time;

    # slurp in all test data from previous symbolic runs
    for my $dir (glob "klee-out-*") {
        print "processing directory $dir\n" if $debug;
        my $TEMP_DIR = "/tmp/bipath-temp";

        # helper: convert query form
        unlink (glob ("$TEMP_DIR/*"));
        my $bipath_start_time = time;
        system("$BIPATH_HELPER $dir $TEMP_DIR");
        my $bipath_time = time - $bipath_start_time;
        print "BipathTime(bipath): $bipath_time\n" if $with_time;
        
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
                            my $padding = $array->{'size'} - length($arg);
                            for (1..$padding) {
                                push @numbers, 0;  # null terminator, padding
                            }

                            $array->{'init'} = "[" . join(' ', @numbers) . "]";
                            $found = 1;
                        }
                    }
                }

                $suitable = 0 if !$found;
                #last if !$suitable;  # keep going in case arg_count makes it suitable
            }

            if(open(COUNT, "<$dir/arg_count")) {
                print "        reading [$dir/arg_count]\n" if $debug;
                my $s = 1;
                for my $a (0..$#args) {
                    my $count = <COUNT>;
                    if(!defined($count)) {
                        $s = 0;
                        last;
                    }
                    chomp $count;
                    if(length($args[$a]) > int $count) {
                        $s = 0;
                        last;
                    }
                }
                $suitable = 1 if $s == 1;
                close COUNT;
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
                            my $kleaver_time = time - $kleaver_start_time;
                            print "BipathTime(kleaver): $kleaver_time\n" if $with_time;
                            return 1;   # success! satisfies the constraints
                        }
                    }
                }
                close KLEAVER;
            }
        }
    }

    my $kleaver_time = time - $kleaver_start_time;
    print "BipathTime(kleaver): $kleaver_time\n" if $with_time;
    print "*** RUNNING IN KLEE\n" if $debug;
    return 0;
}

sub promote_args(@) {
    my @out = ();
    for my $a (@_) {
        push @out, "--sym-arg", length($a);
    }
    return @out;
}
