void idle_thread() {
    while (1) {
        asm("hlt");
    }
}
