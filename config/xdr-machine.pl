#!/usr/bin/perl
#
# Customize machine dependet .h files
# second run of ./configure, after config.h is created
# reason: I do not want any .h files to depend upon config.h
# and I do not want to rewrite all the compiler M4 stuff 
#
# expects config.h as first argument
#
# reads *.h.in files from stdin
# creates *.h on stdout
#


$cfg = shift;
die "Please specify config.h to read as first parameter" unless(-f $cfg);

my %defs;
my $ident = qr{[A-Za-z_][A-Za-z_0-9]*};

open CFG, "$cfg" or die "Cannot open $cfg for read";
while(<CFG>){
    $_ =~ s/[ \t\r\n]+/ /g; 
    next unless($_ =~ /^ *\# *define ($ident) (.*[^ ]) *$/);
    $defs{$1} = $2;

#    print "$1 -> $2\n";
}
close CFG;

sub write_xdr_type {
    my ($type, $cfg, $def) = @_;
    if(defined($cfg)){
	print "#define $type $cfg \n";
    } else {
	print "/* check configure script results, this is a guess */\n";
	print "#define $type $def\n";
    }
}

sub xdr_types {
    my @floats;
    my @ints;
    while( my ($k,$v) = each(%defs) ){
	next unless($k =~ /^SIZEOF_(.*)$/);
	my $type = lc($1);
	$type =~ s/_/ /g;
	$type =~ /^(.* )?([^ ]+)$/;
	my $cls = undef;
	if(($2 eq "float") || ($2 eq "double")){
	    $cls = \@floats;
	} elsif (($2 eq "char") || ($2 eq "short") ||
		 ($2 eq "int")  || ($2 eq "long")){
	    $cls = \@ints;
	}
	next unless(defined($cls));
	my $lg = 0;
	my $lv = 1;
	while(($v - $lv) > 0){
	    $lv *= 2;
	    $lg += 1;
	}
	next unless($v == $lv);
	next if(defined($cls->[$lg]));
	$cls->[$lg] = $type;
    }
    print "/* 8 bit integer (octet) */\n";
    write_xdr_type("XDR_CHAR",$ints[0],"char");
    print "/* 16 bit integer */\n";
    write_xdr_type("XDR_SHORT",$ints[1],"short");
    print "/* 32 bit integer */\n";
    write_xdr_type("XDR_INT",$ints[2],"int");
    print "/* 64 bit integer */\n";
    write_xdr_type("XDR_LONG",$ints[3],"long long");
    print "\n";
    print "/* 32 bit float */\n";
    write_xdr_type("XDR_FLOAT",$floats[2],"float");
    print "/* 64 bit float */\n";
    write_xdr_type("XDR_DOUBLE",$floats[3],"double");
}

sub xdr_endian {
    print "/* Machine endianess */\n";
    if(exists $defs{'WORDS_BIGENDIAN'}){
	print "#define XDR_BIG_ENDIAN\n";
    } else {
	print "#define XDR_LITTLE_ENDIAN\n";
    }
}

while(<>){
    while($_ =~ /@($ident)@/){
	if(exists $defs{$1}){
	    $_ =~ s/@$1@/$defs{$1}/;
	} elsif($1 eq "XDR_TYPES") {
	    xdr_types;
	    $_ = "";
	} elsif($1 eq "XDR_ENDIAN") {
	    xdr_endian;
	    $_ = "";
	} else {
	    print STDERR "Variable @$1@ undefined\n";
	    $_ =~ s/@$1@//;	    
	}
    }
    print $_;
}
