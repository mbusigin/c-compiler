/* Expected diagnostic: functions cannot return functions. */
int f(void)(void);

int main(void) {
    return 0;
}
