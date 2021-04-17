#!/usr/bin/env bash

DOC_PATH="docs/call_graph"
DOC_PATH_DOT="docs/call_graph/dot"
SOURCE_CODE=$@

gen_call_graph () {
    for var in $SOURCE_CODE
    do
        echo "[INFO] gen call graph for: $var"
        cflow2dot -i $var -f pdf -m -o $DOC_PATH/$(basename -- $var)
    done

    echo "[INFO] move dot files to $DOC_PATH_DOT"
    if [[ -p "$DOC_PATH_DOT" ]]; then
        mv $DOC_PATH/*.dot $DOC_PATH_DOT
    else
        mkdir -p $DOC_PATH_DOT
        mv $DOC_PATH/*.dot $DOC_PATH_DOT
    fi
}

##############################################################################
# script start
##############################################################################
if [[ -d "$DOC_PATH" ]]; then
    gen_call_graph
else
    while true; do
        read -p "make dir $DOC_PATH?  (y/n)" yn
        case $yn in
            [Yy]* )  echo "[INFO]make dir $DOC_PATH"; mkdir -p $DOC_PATH; break;;
            [Nn]* )  echo "[INFO]Cancel gen_call_graph" ; exit;;
            * ) echo "Please answer yes or no.";;
        esac
    done
    gen_call_graph
fi


