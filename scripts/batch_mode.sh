#test bash, very rough need to get paths correct first, not quite sure about the layout
current_macro_path=$1

if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS requires an explicit empty string for in-place editing
        sed -i '' \
          -e "s|/vis/.*|#/vis/.*|" \
          "$current_macro_path"
    else
        # Standard Linux/GNU environment format
        sed -i \
          -e "s|/vis/.*|#/vis/.*|" \
          "$current_macro_path"
    fi

#not quite but essentially just comment out and then run the opposite, write this so it can go both ways
