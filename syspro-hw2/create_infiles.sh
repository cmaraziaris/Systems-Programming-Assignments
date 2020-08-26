#!/bin/bash

RANDOM=$$  # Initialize with a random seed

###################################################################
# Argument parse & error handle
USAGE="./create_infiles.sh diseasesFile countriesFile input_dir numFilesPerDirectory numRecordsPerFile"

if (("$#" != 5)); then
  echo $'\n'"[ERROR 5] Please give *exactly* 5 arguments."
  echo "> Usage: $USAGE" $'\n'
  exit 5
fi

DISEASE_FILE=$1
if [ ! -e $DISEASE_FILE ]; then
  echo $'\n'"[ERROR 1] Disease file \"$DISEASE_FILE\" doesn't exist."
  echo "> Usage: $USAGE" $'\n'
  exit 1
fi

COUNTRY_FILE=$2
if [ ! -e $COUNTRY_FILE ]; then
  echo $'\n'"[ERROR 2] Country file \"$COUNTRY_FILE\" doesn't exist."
  echo "> Usage: $USAGE" $'\n'
  exit 2
fi

INPUT_DIR=$3  # No need to check; if it exists, it will be erased & replaced

FILES_PER_DIR=$4
if (("$FILES_PER_DIR" <= 0)); then
  echo $'\n'"[ERROR 3] Please give a positive number of files per directory."
  echo "> Usage: $USAGE" $'\n'
  exit 3
fi

RECORDS_NUM=$5
if (("$RECORDS_NUM" <= 0)); then
  echo $'\n'"[ERROR 4] Please give a positive number of records per file."
  echo "> Usage: $USAGE" $'\n'
  exit 4
fi

###################################################################

ALPHABET=(A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
ACTION=(ENTER EXIT)
RECORD_ID=0

create_name() # Stores a name of 3-12 chars in $NAME variable
{
  NAME=""
  NUM_CHARS=$(( RANDOM % 12 + 3 ))
  for (( j = 0; j < NUM_CHARS; j++ )); do
    LETTER=$(( RANDOM % 26))
    NAME+=${ALPHABET[$LETTER]}
  done
}

create_record()
{
  create_name
  FNAME=$NAME
  create_name
  LNAME=$NAME

  RECORD="$RECORD_ID ${ACTION[$((RANDOM % 2))]} $FNAME $LNAME ${DISEASES[$((RANDOM % $DIS_NUM ))]} $((RANDOM % 120 + 1))"
  let "RECORD_ID = RECORD_ID + 1"
}

###################################################################

read_diseases() # Reads the diseases from file in arg #1
{               # Stores them in array $DISEASES
  DISEASES=()
  while read line; do
    DISEASES+=($line)
  done < $DISEASE_FILE
  DIS_NUM=${#DISEASES[@]}
}

read_countries() # Reads the countries from file in arg #2
{                # Stores them in array $COUNTRIES
  COUNTRIES=()
  while read line; do
    COUNTRIES+=($line)
  done < $COUNTRY_FILE
  CTRY_NUM=${#COUNTRIES[@]}
}

create_dmy() # Generate a random DD or MM or YYYY
{
  DMY=$((RANDOM % $1 + $2))
  while [ ${#DMY} -ne $3 ]; do
    DMY="0"$DMY   # Pad leading 0s
  done
}

create_date() # Creates a random date in DD-MM-YYYY format
{
  create_dmy 30 1 2  # Generate DAY
  DAY=$DMY
  create_dmy 12 1 2  # Generate MONTH
  MONTH=$DMY
  create_dmy 100 2000 4  # Generate YEAR
  YEAR=$DMY
  DATE="$DAY-$MONTH-$YEAR"
}

create_dirs()
{
  if [ -d $INPUT_DIR ]; then  # if DIR exists, delete it
    rm -rf $INPUT_DIR
  fi

  mkdir $INPUT_DIR  # Create input dir
  cd $INPUT_DIR

  for country in ${COUNTRIES[*]}; do  # Create country dirs
    mkdir $country
    cd $country
    for (( m = 0; m < FILES_PER_DIR; m++ )); do  # Create date files
      create_date
      if [ -e $DATE ]; then     # Check if file exists
        while [ -e $DATE ]; do  # Create a date until it *doesn't* exist
          create_date
        done
      fi

      for (( l = 0; l < RECORDS_NUM; l++ )); do  # Fill the date files with records
        create_record
        echo $RECORD >> $DATE
      done
    done
    cd ..
  done
  cd ..  # Return to the initial dir 
}

###################################################################
# Main functionality
read_diseases
read_countries
create_dirs
exit 0

###################################################################