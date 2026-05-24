# KeyFlow Designer

Visual keyboard configuration designer for KeyFlow - a privacy-first keyboard remapper for Windows.

## Features

- 🎨 **Visual Keyboard Editor** - Click keys to remap them
- 🌓 **Dark/Light Theme** - Both themes fully supported with Tailwind CSS
- 📐 **Quad View** - See up to 4 layers per key simultaneously
- 🔄 **Layer Management** - Create and manage multiple layers (up to 4 recommended)
- 📤 **Export/Import** - Save and load `config.json` files
- 🎯 **Schema Compliant** - Follows KeyFlow's configuration schema exactly

## Getting Started

### Development

```bash
# Install dependencies
npm install

# Run development server
npm run dev

# Open http://localhost:3000
```

### Build

```bash
# Build for production
npm run build

# Start production server
npm start
```

## Design

The designer features a VIA-inspired interface with:

- **Single Layer View** - Edit one layer at a time with focused UI
- **All Layers View** - See all 4 layers on each key (cheat sheet mode)
- **Layer Tabs** - Quick switching between BASE and L1-L3
- **Key Picker** - Visual grid organized by category (Basic, Modifiers, Navigation, Numbers, Symbols)

### Theme Support

Both light and dark themes are fully supported using Tailwind CSS:
- Dark theme (default): VIA-inspired color scheme
- Light theme: Clean, accessible alternative
- Toggle via sun/moon icon in header

## Tech Stack

- **Framework**: Next.js 15 (App Router)
- **Styling**: Tailwind CSS with custom dark/light themes
- **State**: Zustand (lightweight global state)
- **Icons**: Lucide React
- **TypeScript**: Full type safety

## Project Structure

```
designer/
├── app/
│   ├── page.tsx          # Main designer page
│   ├── layout.tsx        # Root layout with theme
│   └── globals.css       # Global styles
├── components/
│   ├── Header.tsx        # Top bar with theme toggle
│   ├── ThemeProvider.tsx # Theme context
│   ├── ViewModeToggle.tsx
│   ├── Keyboard/
│   │   ├── Keyboard.tsx  # Main keyboard container
│   │   ├── Key.tsx       # Individual key with quad view
│   │   └── LayerTabs.tsx # Layer switching tabs
│   └── KeyPicker/
│       └── KeyPicker.tsx # Key selection panel
├── lib/
│   ├── store.ts          # Zustand state management
│   ├── types.ts          # TypeScript types
│   ├── utils.ts          # Utility functions
│   └── keyboardLayout.ts # ANSI keyboard layout data
└── tailwind.config.ts    # Tailwind with custom theme
```

## Configuration

The designer generates standard KeyFlow `config.json` files:

```json
{
  "version": "1.0",
  "name": "My Custom Layout",
  "disableCapsLock": true,
  "remapping": {
    "CapsLock": "LeftCtrl"
  },
  "layers": [
    {
      "name": "Numpad Layer",
      "triggers": ["RightAlt"],
      "mappings": {
        "K": "1",
        "L": "2"
      }
    }
  ]
}
```

## Usage Flow

1. **Open Designer** → See blank keyboard with BASE layer
2. **Click a key** → Select it (purple highlight)
3. **Click target key** → Mapping created instantly
4. **Switch layers** → Use tabs to edit different layers
5. **Toggle view mode** → See all layers at once (cheat sheet)
6. **Export** → Download `config.json` for KeyFlow

## Layer Limits

- **Recommended**: 4 layers (BASE + 3 modifier layers)
- **Expandable**: Can add more with warning prompt
- **Physical limit**: ~7-8 practical modifier keys available

## Contributing

This is part of the KeyFlow project. See main repository for contribution guidelines.

## License

MIT - Same as KeyFlow
