
.PATH: ${.CURDIR}

all: test_words

# Retrieve the gzipped dataset.
enron.tar.gz :
	curl https://www.cs.cmu.edu/~enron/enron_mail_20150507.tar.gz --output ${.TARGET}

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
word_histogram : histogram.sh enron.tar
	${.ALLSRC} ${.TARGET}

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
length_count_representatives : representatives.sh word_histogram
	${.ALLSRC} ${.TARGET}

# Sample representitives into a format that can be automated over.
# Each line in this file is a single field:
# 	String, the word to test.
# Steps to reproduce:
# 	Select 1000 candidates from previous dataset 
# 	Select only the word column.
test_words : sample.sh length_count_representatives
	${.ALLSRC} ${.TARGET}

.include <bsd.obj.mk>
