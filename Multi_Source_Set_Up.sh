# Imma call this file and then for each in

#ensure correct input: csv file and a number of events needed, add proper bool here
if [[ -z "$1" && -z "$2" ]]; then
    echo "Error: You must supply number of events (int) and CSV file."
    echo "Usage: ./Multi_Source_Set_Up.sh <int> <.csv>"
    exit 1
fi

[ -f template_multi_source.mac ] && cp template_multi_source.mac template_multi_source.mac.bak

Event_Number=$1
csv_file=$2

while IFS=',' read -r col1 col2 col3 col4; do
    # ignore header to csv
    if [[ "$col1" == "type" ]]; then
        continue
    fi

    [ -f template_source.mac ] && cp template_source.mac template_source.mac.bak

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
      -e "s|/gps/source/add.*|/gps/source/add $intensity|" \
      template_source.mac

    #if else here to make it so first added source is just added otherwise sourceadd used instead.
    
    if []; then
        seq $num | xargs -I {} echo "/control/execute template_source.mac" >> template_multi_source.mac
    else
    fi

    # clean up the copy to reset for the next loop, pulled over from other file.
    if [ -f template_source.mac.bak ]; then
        mv template_source.mac.bak template_source.mac
    else
        rm -f template_source.mac
    fi
done < "$csv_file"

echo "/gps/source/multiplevertex true" >> template_multi_source.mac.bak
echo "/run/beamOn $Event_Number" >> template_multi_source.mac.bak

if [ -f template_multi_source.mac.bak ]; then
    mv template_multi_source.mac.bak template_multi_source.mac
else
    rm -f template_multi_source.mac
fi