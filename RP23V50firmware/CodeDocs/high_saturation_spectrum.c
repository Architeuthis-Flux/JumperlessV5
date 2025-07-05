// High-saturation colors from 6x6x6 cube arranged in spectrum order
// Filtered for saturation >= 0.5 and value >= 0.2
// Total colors: 54 out of 216 possible
const int highSaturationSpectrumColors[54] = {
  // Red hues (0-30°)
  160, 196, 202, 166,
  // Orange hues (30-60°)
  208, 172, 214, 178, 220,
  // Yellow hues (60-90°)
  184, 226, 190, 148, 154, 112, 118,
  // Yellow-Green hues (90-120°)
  76, 82,
  // Green hues (120-150°)
  40, 46, 47, 41,
  // Green-Cyan hues (150-180°)
  48, 42, 49, 43, 50,
  // Cyan hues (180-210°)
  44, 51, 45, 38, 39, 32, 33,
  // Cyan-Blue hues (210-240°)
  26, 27,
  // Blue hues (240-270°)
  20, 21, 57, 56,
  // Blue-Magenta hues (270-300°)
  93, 92, 129, 128, 165,
  // Magenta hues (300-330°)
  164, 201, 200, 163, 199, 162, 198,
  // Red-Magenta hues (330-360°)
  161, 197
};

const int highSaturationSpectrumColorsCount = 54;