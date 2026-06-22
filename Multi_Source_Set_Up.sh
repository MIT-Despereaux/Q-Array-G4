# Imma call this file and then for each in

#ensure correct input: csv file and a number of events needed, add proper bool here

[ -f template_multi_source.mac ] && cp template_multi_source.mac template_multi_source.mac.bak

Event_Number=$1
for entry in in_list:
    [ -f template_source.mac ] && cp template_source.mac template_source.mac.bak

    particletype = entry[0]
    num = entry[1]
    intensity = entry[2]
    spectrum = entry[3]
    # edit copy of the template
    #if else here to make it so first added source is just added otherwise sourceadd used instead.
    if []; then
        seq num | xargs -I {} echo "/control/execute template_source.mac.bak" >> template_multi_source.mac.bak
    else
    fi

echo "/gps/source/multiplevertex true" >> template_multi_source.mac.bak
echo "/run/beamOn $Event_Number" >> template_multi_source.mac.bak

