package gnuplot;
use strict;
use vars qw (@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION);

use Exporter;
$VERSION = 1.0;
@ISA = qw(Exporter);

@EXPORT = qw(gnuplot);
@EXPORT_OK = qw();
%EXPORT_TAGS = qw();

##############################################
## source 
##############################################

use IO::File;
use POSIX qw(tmpnam);

sub float
{
    my $real='[+-]?\d+\.?\d*[eE]?[+-]?\d+|[+-]?\d*\.?\d+[eE]?[+-]?\d+|[+-]?\d+';
    my (@list,$f,$s,$in);

    if (@_==1) { @list=grep /$real/,split /($real)/,$_[0]; }
    elsif (@_==2)
    {
        $in=' '.$_[1];
        ($f,$s)=split /$_[0]/,$in,2;
        @list=grep /$real/,split /($real)/,$s;
    }
    else
    {
        $in=' '.$_[2];
        ($f,$s)=split /$_[0]/,$in,2;
        @list=grep /$real/,split /($real)/,$s;
        if ($_[1]>=@list || $_[1]<0) { return undef;}
        for ($s=0; $s<$_[1]; $s++) { shift @list; }
    }
    return wantarray ? @list : $list[0];
}

sub gnuplot
{
	my ($command,$pid,$fh);

	# basic check
	if (@_<=0) 
	{
		die "ERROR: provide gnuplot command\n";
	} 
	$command=$_[0];

	# check for I channel
	if ($command ne "start")
	{
		1==stat IN or die "ERROR: IN channel missing\n";
	}

	SWITCH:
	{
		# command 'start'
		if ($command eq "start")
		{
			if(@_!=1 && @_!=2)
        	{   
        	    die "ERROR: provide no or one (command line) option with 'start'\n";
        	} 
        	if (@_==1) { $pid=open(IN,"| gnuplot"); }
        	if (@_==2) { $pid=open(IN,"| gnuplot $_[1]"); }
			$fh=select(IN);
			$|=1;
			select($fh);
			return $pid;
		}

		# command 'end'
        if ($command eq "end")
        {
            if(@_!=1)
            {
                die "ERROR: provide no option with 'end'\n";
            }
            close(IN);
			return;
        }
	
		# command 'write'
        if ($command eq "write")
        {
			my ($s,$i,$j);
			my $n=2;
            if(@_!=3 && @_!=4)
            {
                die "ERROR: usage: gnuplot 'write',<filename>,<array-reference>, [<n>]\n";
            }
			elsif (@_==4)
			{
				$n=$_[3];
			}
			open(FILE,">$_[1]");
			$s=$_[2];
			for ($i=0; $i<@$s; $i+=$n) 
			{
				for ($j=$i; $j<$i+$n; $j++)
				{ 
					print FILE "$$s[$j] "; 
				}
				print FILE "\n";
			}
			close(FILE);
			
			return;
        }
	
		# std command
        if (@_==1)
        {
			print IN "$_[0]\n";
            return;
		}
		if (@_>=2)
		{
			my ($i,$j,$k,$s,@a,$t,$name);
			@a=split //,$_[0]; 
			for ($j=0,$k=1; $j<@a; $j++)
			{
				if ($a[$j] eq '%')
				{
					$name="/tmp/gnu_$k"; 
					$a[$j]='"'.$name.'"';
					$s=$_[$k++];
					open(TMP,">$name");
					for $i (keys %$s) { if ($i ne (float $i) || $$s{$i} ne (float $$s{$i})) { die "ERROR: hash table contains invalid pair '$i $$s{$i}'\n" } }
					for $i (sort {$a<=>$b} keys %$s) { print TMP "$i $$s{$i}\n"; }
					close(TMP);
				}
				if ($a[$j] eq '@')
				{
					$name="/tmp/gnu_$k"; 
					$a[$j]='"'.$name.'"';
					$s=$_[$k++];
					open(TMP,">$name");
					for ($i=0; $i<@$s; $i+=2) { if ($$s[$i] eq '' || $$s[$i+1] eq '') {print TMP "\n"; next; } print TMP "$$s[$i] $$s[$i+1]\n"; }
					close(TMP);
				}
			}
			$t=join '',@a;
			print IN "$t\n";
            return;
		}

	}
}

1;




