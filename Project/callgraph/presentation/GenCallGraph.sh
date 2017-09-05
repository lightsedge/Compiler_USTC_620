infilecc=$(ls ./ExampleCode/*.cc)

callgraph $infilecc $infilec -root-function=./ExampleCode/cgen-phase.cc:main
dot main.dot -Tsvg -o main.svg
