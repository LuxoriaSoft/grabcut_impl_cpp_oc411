using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Luxoria.Algorithm.GrabCut;

public class GrabCut
{
    private const string NativeLibraryName = "extract_fg";

    static GrabCut()
        => ExtractAndLoadNativeLibrary();

    public void Exec(string inputFile, string outputfile, int x, int y, int width, int height, int margin = 0, bool color = true, Color? foreground = null, Color? background = null)
    {
        if (!color && (foreground == null || background == null))
            throw new ArgumentException("Foreground and background colors must be provided when alive color is disabled");

        int fR = foreground?.R ?? 0;
        int fG = foreground?.G ?? 0;
        int fB = foreground?.B ?? 0;
        int bR = background?.R ?? 0;
        int bG = background?.G ?? 0;
        int bB = background?.B ?? 0;

        int status = grabcut_exec(inputFile, outputfile, x, y, width, height, margin, color, fR, fG, fB, bR, bG, bB);

        if (status != 0)
        {
            throw new InvalidOperationException($"GrabCut execution failed code : {status}");
        }
    }

    #region P/Invoke DLL Loader System

    [DllImport(NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int grabcut_exec(string imagePath, string outputFile, int x, int y, int width, int height, int margin,
        bool color, int fR, int fG, int fB, int bR, int bG, int bB);

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
