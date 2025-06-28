#include "engine/debug.h"
#include "stdlib.h"
int main(){
    ChessPosition cp=alloca(sizeof(struct chess_position));
    init_position(cp);
    print_position(cp);
}