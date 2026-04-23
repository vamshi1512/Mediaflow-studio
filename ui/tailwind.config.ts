import type { Config } from "tailwindcss";

export default {
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        shell: "#081018",
        panel: "#111a26",
        panelAlt: "#182435",
        accent: "#4dd2ff",
        coral: "#ff7a66",
        lime: "#d7ff64",
        violet: "#8fb6ff",
        chrome: "#b4c2d3",
      },
      boxShadow: {
        glow: "0 20px 80px rgba(77, 210, 255, 0.16)",
      },
      fontFamily: {
        display: ["Space Grotesk", "sans-serif"],
        body: ["IBM Plex Sans", "sans-serif"],
      },
      backgroundImage: {
        "dashboard-grid":
          "linear-gradient(rgba(180, 194, 211, 0.08) 1px, transparent 1px), linear-gradient(90deg, rgba(180, 194, 211, 0.08) 1px, transparent 1px)",
      },
    },
  },
  plugins: [],
} satisfies Config;
