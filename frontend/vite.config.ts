import tailwindcss from "@tailwindcss/vite";
import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";

// The UI runs on 5173 and the API on 5174. Anything the browser asks for under
// /api is forwarded to the API server, so the front end can use plain relative
// URLs and there is no CORS to configure.
export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    port: 5173,
    proxy: {
      "/api": "http://localhost:5174",
    },
  },
  build: {
    outDir: "dist",
  },
});
