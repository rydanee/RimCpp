compilation guide (linux only for now but you can try doing it manually for windows ig)

```
cmake -B build .
cmake --build ./build/
```

the executable will be located at ./build/RimCpp
compilation for the first time might be slow due to RayLib being downloaded
