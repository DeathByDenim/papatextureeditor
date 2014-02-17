Papa Texture Editor
====
### Description
Editor for the papa texture for Planetary Annihilation.
So far you can view all of the textures. Saving them into the papa file is only possible for A8R8B8G8 and X8R8B8G8. Still working on the others, but you can also use the tool papatran that comes with Planetary Annihilation.

### Compilation
To compile yourself:

#### For Linux:
```
mkdir build
cd build
cmake ..
make
```

#### For Mac:
```
cmake -G Xcode
xcodebuild -project papatextureeditor.xcodeproj -target papatextureeditor -configuration Release
```

#### For Windows (using MinGW):
```
PATH=%PATH%;C:\MinGW\bin;C:\Qt\4.8.5\bin
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
make
```
