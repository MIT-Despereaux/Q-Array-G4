#!/bin/bash

#ensure correct input: csv file and a number of events needed, add proper bool here
if [[ -z "$1" || -z "$2" ]]; then
    echo "Error: You must supply number of events (int) and CSV file."
    echo "Usage: ./Multi_Source_Set_Up.sh <int> <.csv>"
    exit 1
fi

# backup
[ -f template_multi_source.mac ] && cp template_multi_source.mac template_multi_source.mac.orig

Event_Number=$1
csv_file=$2
first_source=true
row_index=0

while IFS=',' read -r col1 col2 col3 col4 || [ -n "$col1" ]; do
    # clean from Excel/Sheets exports
    col1=$(echo "$col1" | tr -d '\r\n[:space:]')
    col2=$(echo "$col2" | tr -d '\r\n[:space:]')
    col3=$(echo "$col3" | tr -d '\r\n[:space:]')
    col4=$(echo "$col4" | tr -d '\r\n[:space:]')

    ((row_index++))

    # ignore header to csv
    if [ $row_index -eq 1 ]; then
        continue
    fi

    
    if [[ -z "$col1" ]]; then continue; fi

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

    # modify the files here per each source
    source_block=$(sed \
      -e "s|/gps/particle.*|/gps/particle $particletype|" \
      -e "s|/control/execute.*|/control/execute $spectrum|" \
      template_source.mac)
    
    #if else here to make it so first added source is just added otherwise sourceadd used instead.
    
    if [ "$first_source" = true ]; then
        
        echo "/gps/source/intensity $intensity" >> template_multi_source.mac
        echo "$source_block" >> template_multi_source.mac
        echo "" >> template_multi_source.mac  # refreshing

        if [ "$num" -gt 1 ]; then
            run_num=$((num-1))
            for i in $(seq $run_num); do
                echo "/gps/source/add $intensity" >> template_multi_source.mac
                echo "$source_block" >> template_multi_source.mac
                echo "" >> template_multi_source.mac  
            done
        fi
        first_source=false
    else
        for i in $(seq $num); do
            echo "/gps/source/add $intensity" >> template_multi_source.mac
            echo "$source_block" >> template_multi_source.mac
            echo "" >> template_multi_source.mac  
        done
    fi

done < "$csv_file"

echo "" >> template_multi_source.mac
echo "/gps/source/multiplevertex true" >> template_multi_source.mac

# --- MODIFIED: Only auto-fire if we aren't trying to inspect the GUI ---
# Comment out or remove the forced beamOn command to keep the GUI open:
# echo "/run/beamOn $Event_Number" >> template_multi_source.mac

#now slide it into init_vis.mac in similiar manner.
echo "/control/execute ./template_multi_source.mac" >> init_vis.mac

# restore
mv template_multi_source.mac.orig template_multi_source.mac

echo "Finished subscript!"