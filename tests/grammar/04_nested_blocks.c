int main() {
    int outer = 1;
    {
        int middle = 2;
        {
            int inner = 3;
            outer = outer + middle + inner;
        }
    }
    return outer;
}
