int oneline() { test; }

int continuesameline(a) {
    if (a ? a : 0) continue;
}

int derefloop() {
    int val = a ? *a : 0;

    for (;;)
        return a ? *a : 0;
}

void tst(int z)
{
  f = R(z);
}

void test(int z)
{
  wchar_t * str = L"abcd";
  R(z);
}
