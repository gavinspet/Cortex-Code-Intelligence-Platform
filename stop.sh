#!/bin/bash
pkill -f 'build/bin/cortex' 2>/dev/null && echo "Stopped: cortex backend" || echo "backend not running"
pkill -f 'vite'             2>/dev/null && echo "Stopped: vite frontend" || echo "frontend not running"
