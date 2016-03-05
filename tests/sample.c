int oneline() { test; }

int continuesameline(a) {
    if (a ? a : 0) continue;
}

int derefloop() {
    int val = a ? *a : 0;

    for (;;)
        return a ? *a : 0;
}
