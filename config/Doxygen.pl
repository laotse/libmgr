#!/usr/bin/perl
#
# Set Parameters in Doxyfile
#

sub consume_lines {
    my $line = shift;
    while ( $line =~ /\\$/ ){
	$line = <>;
    }
}

my $root_path = shift;
my $input_file = shift;
my $output_dir = shift;
my $exclude_file = shift;

while ( <> ) {
    if( $_ =~ /^STRIP_FROM_PATH[ \t]*=/ ){
	consume_lines $_;
	print "STRIP_FROM_PATH = $root_path\n";
    } elsif( $_ =~ /^OUTPUT_DIRECTORY[ \t]*=/ ){
	consume_lines $_;
	print "OUTPUT_DIRECTORY = $output_dir\n";
    } elsif( $_ =~ /^INPUT[ \t]*=/ ){
	consume_lines $_;
	print "INPUT =";
	open( IFIL, "$input_file" ) or die "Cannot open $input_file";
	while(<IFIL>){
	    chomp;
	    print " \\\n" . $_;
	}
	close IFIL;	
	print "\n";
    } elsif( $_ =~ /^RECURSIVE[ \t]*=/ ){
	consume_lines $_;
	print "RECURSIVE = NO\n";
    } elsif ( $_ =~ /^EXCLUDE[ \t]*=/ ){
	my $line = $_;
	while ( $line =~ /\\$/ ){
	    print $line;
	    $line = <>;
	}
	if( ! -f "$exclude_file" ){
	    print $line;
	} else {
	    chomp $line;
	    print $line;
	    open( IFIL, "$exclude_file" ) or die "Cannot open $exclude_file";
	    while(<IFIL>){
		chomp;
		print " \\\n" . $_;
	    }
	    close IFIL;	
	    print "\n";
	}	
    } else {
	print $_;
    }
}
