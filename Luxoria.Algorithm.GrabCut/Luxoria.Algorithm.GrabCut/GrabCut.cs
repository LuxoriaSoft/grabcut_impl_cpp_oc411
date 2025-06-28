using System.Reflection;
using System.Runtime.InteropServices;

namespace Luxoria.Algorithm.GrabCut;

public class GrabCut
{
    private const string NativeLibraryName = "extract_fg";

    static GrabCut()
        => ExtractAndLoadNativeLibrary();

    public void Exec(string inputFile, string outputfile, int x, int y, int width, int height, int margin = 0)
    {
        int status = grabcut_exec(inputFile, outputfile, x, y, width, height, margin);

        if (status != 0)
        {
            throw new InvalidOperationException($"GrabCut execution failed code : {status}");
        }
    }

    #region P/Invoke DLL Loader System

    [DllImport(NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int grabcut_exec(string imagePath, string outputFile, int x, int y, int width, int height, int margin);

    /// <summary>
    /// Extracts and loads the library from embedded resources depending on the Runtime arch
    /// </summary>
    /// <exception cref="NotSupportedException">Architecture not being supported</exception>
    /// <exception cref="FileNotFoundException">Cannot find the specified file</exception>
    private static void ExtractAndLoadNativeLibrary()
    {
        string architecture = RuntimeInformation.ProcessArchitecture switch
        {
            Architecture.X86 => "x86",
            Architecture.X64 => "x64",
            Architecture.Arm64 => "arm64",
            _ => throw new NotSupportedException("Unsupported architecture")
        };

        string resourceName = $"Luxoria.Algorithm.GrabCut.NativeLibraries.{architecture}.{NativeLibraryName}.dll";

        string tempPath = Path.Combine(Path.GetTempPath(), "LuxoriaNative");
        Directory.CreateDirectory(tempPath);

        string dllPath = Path.Combine(tempPath, $"{NativeLibraryName}.dll");

        // Extract the DLL from embedded resources
        using (Stream? resourceStream = Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName))
        {
            if (resourceStream == null)
                throw new FileNotFoundException($"Embedded resource not found: {resourceName}");

            using (FileStream fileStream = new FileStream(dllPath, FileMode.Create, FileAccess.Write))
            {
                resourceStream.CopyTo(fileStream);
            }
        }

        // Load the extracted native library
        NativeLibrary.Load(dllPath);
    }

    #endregion
}
