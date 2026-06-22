#!/bin/bash

#ensure correct input: csv file and a number of events needed, add proper bool here
if [[ -z "$1" || -z "$2" ]]; then
    echo "Error: You must supply number of events (int) and CSV file."
    echo "Usage: ./Multi_Source_Set_Up.sh <int> <.csv>"
    exit 1
fi

# TWEAK HERE: Backup your clean template first!
[ -f template_multi_source.mac ] && cp template_multi_source.mac template_multi_source.mac.orig

Event_Number=$1
csv_file=$2
first_source=true

# Keep a clean original copy before entering the loop so it doesn't get overwritten with row data
[ -f template_source.mac ] && cp template_source.mac template_source.mac.orig

while IFS=',' read -r col1 col2 col3 col4; do
    # ignore header to csv
    if [[ "$col1" == "type" ]]; then
        continue
    fi

    # variables Imma use go here:
    particletype="$col1"
    declare -i num="$col2" 
    #gotta make sure integer number of sources
    intensity="$col3"
    spectrum="$col4"

    # read back so i can sanity check.
    echo "New Source"
    echo "Type: $particletype"
    echo "Number:  $num"
    echo "Intensity: $intensity"
    echo "--------------------------"

    # edit copy of the template
    sed -i \
      -e "s|/gps/particle.*|/gps/particle $particletype|" \
      -e "s|/control/execute.*|/control/execute $spectrum|" \
      template_source.mac

    #if else here to make it so first added source is just added otherwise sourceadd used instead.
    
    if [ "$first_source" = true ]; then
        
        echo "/gps/source/intensity $intensity" >> template_multi_source.mac
        echo "/control/execute template_source.mac" >> template_multi_source.mac

        if [ "$num" -gt 1 ]; then
            run_num=$((num-1))
            seq $run_num | xargs -I {} printf "/gps/source/add $intensity\n/control/execute template_source.mac\n" >> template_multi_source.mac
        fi
        first_source=false
    else
        seq $num | xargs -I {} printf "/gps/source/add $intensity\n/control/execute template_source.mac\n" >> template_multi_source.mac
    fi

    # clean up the copy to reset for the next loop, pulled over from other file.
    cp template_source.mac.orig template_source.mac
done < "$csv_file"

rm -f template_source.mac.orig

echo "/gps/source/multiplevertex true" >> template_multi_source.mac
echo "/run/beamOn $Event_Number" >> template_multi_source.mac

#now slide it into init_vis.mac in similiar manner.
echo "/control/execute ./template_multi_source.mac" >> init_vis.mac

# TWEAK HERE: Restore the multi_source template back to its pristine state for your next execution block
mv template_multi_source.mac.orig template_multi_source.mac

echo "Finished subscript!"