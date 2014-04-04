sed 's/defaultOS = .*,/defaultOS = '$OS',/g' $1 | \
sed 's/defaultWordsize = .*,/defaultWordsize = '$WORDSIZE',/g'