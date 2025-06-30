# GrabCut Implementation in .NET
## OpenCV GrabCut Algorithm

This package provides a .NET wrapper for the GrabCut algorithm, implemeted in native C++ with OpenCV.

## Requirements
- **.NET Version**: `net8.0` or compatible.
- **Native Dependencies**: OpenCV 4.10.0 libraries are embedded within the native implementation.

## Source Code
The precompiled native libraries are built from the source code available at [LuxoriaSoft/grabcut_oc4100_dllcore](https://github.com/LuxoriaSoft/grabcut_oc4100_dllcore)

## Installation
You can install the package via NuGet Package Manager or the `.NET CLI`:

### Using NuGet Package Manager
Search for `Luxoria.Algorithm.YoLoDetectModel` in the NuGet Package Manager and install it.

### Using .NET CLI
Run the following command:
```bash
dotnet add package Luxoria.Algorithm.GrabCut --version 1.0.0
```

### Usage
```csharp	
using Luxoria.Algorithm.GrabCut;

class Program
{
    static void Main()
    {
        GrabCut grabCut = new GrabCut();
        grabCut.Exec("image.jpg", "output.jpg", 678, 499, 1653, 1493, 5);
        // Where :
        // "image.jpg" is the input image path,
        // "output.jpg" is the output image path,
        // 678, 499, 1653, 1493 are the rectangle coordinates (x, y, width, height),
        // 5 is the margin from (0 to 100) (default set to 0)
    }
}
```

### License
Luxoria.Algorithm.BrisqueScore is licensed under the Apache 2.0 License. See [LICENSE](LICENSE) for more information.

LuxoriaSoft