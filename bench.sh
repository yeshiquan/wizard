./output/bin/bench_hazptr_thread_local -ops=10000 > tc.txt
./output/bin/bench_hazptr_global -ops=10000 > global.txt

echo "tc.txt" && cat tc.txt
echo ""
echo "global.txt" && cat global.txt
