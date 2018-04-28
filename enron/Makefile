
all: test_words

# Retrieve the gzipped dataset.
enron.tar.gz :
	curl https://www.cs.cmu.edu/~enron/enron_mail_20150507.tar.gz --output $@

# Uncompress the data set but leave it in tarred format.
enron.tar : enron.tar.gz
	gunzip enron.tar.gz

# Compute a histogram of "words" in the file.
# Each line in this file has three fields:
#   Number, the number of appearances of the "word"
#   String, the actual contents
# Steps to reproduce:
#   Convert all nonprintable and whitespace characters to newlines.
#   Drop all lines that don't have any content (e.g. adjacent unprintable characters or spaces in the original data set)
#   Sort these lines to collect similar lines adjacent to one another
#   Count instances of each unique line.
word_histogram : enron.tar
	tr -c "[[:print:]]" "\n" < $< | tr " \n\t," "\n" |  grep -axv "" | sort | uniq -c > $@

# Select representatives for each pair (length, occurances)
# Each line in this file has four fields, representing:
#   Number, the count of strings in the category
#   String, representitive for this category
#   Number, length of the previous string
#   Number, the number of occurances of the string in the dataset.
# Steps to reproduce:
#   Append the length of each string to the beginning of the field.
#   Sort the list by (length, count)
#   Permute the record to place the actual string at the beginning
#   Count instances that share the same (length, count) parameters, selecting one arbitrarily.
length_count_representatives : word_histogram
	awk "{ print length(\$2), \$1, \$2 }" $< | sort -n | awk "{ print \$3, \$1, \$2 }" - | uniq -cf 2 > $@

# Sample representitives into a format that can be automated over.
test_words : length_count_representatives
	shuffle -f length_count_representatives -p 100 | cut -wf 3 > $@
