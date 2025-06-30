using Luxoria.Algorithm.GrabCut;
using System.Drawing;

GrabCut grabCut = new GrabCut();

// Alive color (real picture colors)
grabCut.Exec("image.jpg", "output.jpg", 678, 499, 1653, 1493, 5);

// Custom fg & bg colors
//grabCut.Exec("image.jpg", "output_custom.jpg", 678, 499, 1653, 1493, 5, false, Color.Black, Color.White);