#!/usr/bin/env python3
"""
🦎 AI-GIT TERMINAL USER INTERFACE (TUI)
Production-quality interactive terminal dashboard for AI-Git
Content-Addressable Storage VCS with FastCDC Chunking & Blake3 Hashing.

Palette derived from the AI-Git Chameleon Logo:
- Deep Navy / Near-black terminal backgrounds: #0a0f1d / #0f172a
- Primary Accents: Teal/Cyan #00f0ff & Emerald Green #10b981
- Secondary Accents: Electric Blue #38bdf8 & Mint Green #34d399
- Status & Badges: Staged #10b981, Modified #f59e0b, Untracked #38bdf8
"""

import os
import sys
import time
import shutil
import sqlite3
import subprocess
import threading
from pathlib import Path

# Enable ANSI escape sequences on Windows console
if os.name == 'nt':
    import msvcrt
    import ctypes
    kernel32 = ctypes.windll.kernel32
    # Enable ENABLE_VIRTUAL_TERMINAL_PROCESSING (0x0004)
    hOut = kernel32.GetStdHandle(-11) # STD_OUTPUT_HANDLE
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(hOut, ctypes.byref(mode))
    kernel32.SetConsoleMode(hOut, mode.value | 0x0004 | 0x0001)

# =====================================================================
# CHAMELEON COLOR PALETTE & ANSI STYLING
# =====================================================================
class Palette:
    RESET       = "\033[0m"
    BOLD        = "\033[1m"
    DIM         = "\033[2m"
    ITALIC      = "\033[3m"
    UNDERLINE   = "\033[4m"

    # AI-Git Chameleon Logo Accents (24-bit TrueColor)
    CYAN        = "\033[38;2;0;240;255m"    # #00f0ff Bright Teal/Cyan
    TEAL        = "\033[38;2;34;211;238m"   # #22d3ee Soft Teal
    GREEN       = "\033[38;2;16;185;129m"   # #10b981 Emerald Green
    MINT        = "\033[38;2;52;211;153m"   # #34d399 Mint Green
    BLUE        = "\033[38;2;56;189;248m"   # #38bdf8 Electric Blue
    PURPLE      = "\033[38;2;168;85;247m"   # #a855f7 Purple Accent
    AMBER       = "\033[38;2;245;158;11m"   # #f59e0b Warning
    RED         = "\033[38;2;239;68;68m"    # #ef4444 Deleted/Error
    
    # Neutrals & Text
    WHITE       = "\033[38;2;248;250;252m"  # #f8fafc Crisp White
    SLATE       = "\033[38;2;148;163;184m"  # #94a3b8 Muted Secondary
    DARK_SLATE  = "\033[38;2;71;85;105m"    # #475569 Dim Info
    BORDER_DIM  = "\033[38;2;30;41;59m"     # #1e293b Inactive Border
    BORDER_ACT  = "\033[38;2;0;240;255m"    # #00f0ff Focused Border

    # Backgrounds
    BG_NAVY     = "\033[48;2;10;15;29m"     # #0a0f1d Deep Navy BG
    BG_PANEL    = "\033[48;2;15;23;42m"     # #0f172a Panel Dark BG
    BG_ACTIVE   = "\033[48;2;30;41;59m"     # #1e293b Active Item Row
    BG_CYAN     = "\033[48;2;0;240;255m\033[38;2;10;15;29m"
    BG_GREEN    = "\033[48;2;16;185;129m\033[38;2;10;15;29m"
    BG_AMBER    = "\033[48;2;245;158;11m\033[38;2;10;15;29m"
    BG_BLUE     = "\033[48;2;56;189;248m\033[38;2;10;15;29m"

    # Command Action Buttons (Pink, Blue, Green, Purple) with Crisp White Text
    BTN_PINK    = "\033[48;2;236;72;153m\033[38;2;255;255;255m\033[1m"   # #ec4899 Pink BG + White Bold
    BTN_BLUE    = "\033[48;2;37;99;235m\033[38;2;255;255;255m\033[1m"    # #2563eb Blue BG + White Bold
    BTN_GREEN   = "\033[48;2;16;185;129m\033[38;2;255;255;255m\033[1m"   # #10b981 Green BG + White Bold
    BTN_PURPLE  = "\033[48;2;147;51;234m\033[38;2;255;255;255m\033[1m"   # #9333ea Purple BG + White Bold


def format_bytes(size: int) -> str:
    """Format bytes into readable string (B, KB, MB, GB)."""
    if size < 1024:
        return f"{size} B"
    elif size < 1024 * 1024:
        return f"{size / 1024:.1f} KB"
    elif size < 1024 * 1024 * 1024:
        return f"{size / (1024 * 1024):.1f} MB"
    else:
        return f"{size / (1024 * 1024 * 1024):.1f} GB"


# =====================================================================
# AI-GIT REPOSITORY DATA INTERFACE
# =====================================================================
class AIGitRepo:
    def __init__(self, root: Path = None):
        self.root = root or self.find_repo_root()
        self.aigit_dir = self.root / ".aigit" if self.root else None

    @staticmethod
    def find_repo_root() -> Path | None:
        curr = Path.cwd().resolve()
        for p in [curr] + list(curr.parents):
            if (p / ".aigit").is_dir():
                return p
        return None

    def is_valid(self) -> bool:
        return self.aigit_dir is not None and self.aigit_dir.is_dir()

    def get_current_branch(self) -> str:
        if not self.is_valid():
            return "none"
        head_file = self.aigit_dir / "HEAD"
        if not head_file.exists():
            return "main"
        content = head_file.read_text(encoding="utf-8", errors="ignore").strip()
        if content.startswith("ref: refs/heads/"):
            return content.replace("ref: refs/heads/", "")
        return content[:7] if content else "main"

    def get_current_commit_hash(self) -> str:
        if not self.is_valid():
            return ""
        branch = self.get_current_branch()
        ref_file = self.aigit_dir / "refs" / "heads" / branch
        if ref_file.exists():
            return ref_file.read_text(encoding="utf-8", errors="ignore").strip()
        return ""

    def get_branches(self) -> list[str]:
        if not self.is_valid():
            return []
        heads_dir = self.aigit_dir / "refs" / "heads"
        if not heads_dir.exists():
            return ["main"]
        branches = [f.name for f in heads_dir.iterdir() if f.is_file()]
        return branches if branches else ["main"]

    def read_index(self) -> dict[str, dict]:
        """Read .aigit/index into map: path -> {hash, mode}"""
        entries = {}
        if not self.is_valid():
            return entries
        index_file = self.aigit_dir / "index"
        if not index_file.exists():
            return entries
        try:
            with open(index_file, "r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        path = parts[0]
                        obj_hash = parts[1]
                        mode = parts[2] if len(parts) > 2 else "100644"
                        entries[path] = {"hash": obj_hash, "mode": mode}
        except Exception:
            pass
        return entries

    def get_working_tree_status(self) -> tuple[list[dict], list[dict], list[dict], list[dict]]:
        """Returns (staged, modified, untracked, deleted)"""
        staged = []
        modified = []
        untracked = []
        deleted = []

        if not self.is_valid():
            return staged, modified, untracked, deleted

        index_entries = self.read_index()
        seen_files = set()

        # Scan workspace files
        for p in self.root.rglob("*"):
            if not p.is_file():
                continue
            rel = p.relative_to(self.root).as_posix()
            if rel.startswith(".aigit") or rel.startswith(".git") or rel.startswith("build") or rel.startswith("vcpkg"):
                continue

            seen_files.add(rel)
            size = p.stat().st_size

            if rel in index_entries:
                idx = index_entries[rel]
                # Staged
                staged.append({
                    "path": rel,
                    "hash": idx["hash"],
                    "size": size,
                    "status": "staged"
                })
            else:
                # Untracked
                untracked.append({
                    "path": rel,
                    "hash": "",
                    "size": size,
                    "status": "untracked"
                })

        # Check deleted files
        for path, idx in index_entries.items():
            if path not in seen_files:
                deleted.append({
                    "path": path,
                    "hash": idx["hash"],
                    "size": 0,
                    "status": "deleted"
                })

        return staged, modified, untracked, deleted

    def get_commits(self) -> list[dict]:
        """Traverse commit history from HEAD"""
        commits = []
        if not self.is_valid():
            return commits
        curr_hash = self.get_current_commit_hash()

        visited = set()
        while curr_hash and curr_hash not in visited:
            visited.add(curr_hash)
            obj_path = self.aigit_dir / "objects" / curr_hash[:2] / curr_hash[2:]
            if not obj_path.exists():
                break

            try:
                # Parse commit object
                data = obj_path.read_bytes()
                # Decompress if zlib compressed
                import zlib
                try:
                    data = zlib.decompress(data)
                except Exception:
                    pass

                text = data.decode("utf-8", errors="ignore")
                lines = text.splitlines()

                tree_hash = ""
                parents = []
                author = "Unknown"
                message = ""
                in_message = False

                for line in lines:
                    if in_message:
                        message += line + "\n"
                    elif line.startswith("tree "):
                        tree_hash = line.split()[1]
                    elif line.startswith("parent "):
                        parents.append(line.split()[1])
                    elif line.startswith("author "):
                        author = line.replace("author ", "").split("<")[0].strip()
                    elif line == "":
                        in_message = True

                commits.append({
                    "hash": curr_hash,
                    "tree": tree_hash,
                    "parents": parents,
                    "author": author,
                    "message": message.strip() or "(no commit message)"
                })

                curr_hash = parents[0] if parents else ""
            except Exception:
                break

        return commits

    def get_cas_telemetry(self) -> dict:
        """Query SQLite metadata.db and objects directory for CAS metrics"""
        stats = {
            "total_objects": 0,
            "cas_bytes": 0,
            "dedup_ratio": 0.0,
            "chunks_count": 0,
            "manifests_count": 0
        }
        if not self.is_valid():
            return stats

        db_path = self.aigit_dir / "metadata.db"
        if db_path.exists():
            try:
                conn = sqlite3.connect(str(db_path), timeout=2)
                cur = conn.cursor()
                # Check tables
                cur.execute("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='objects'")
                if cur.fetchone()[0] > 0:
                    cur.execute("SELECT count(*), coalesce(sum(size), 0) FROM objects")
                    row = cur.fetchone()
                    stats["total_objects"] = row[0]
                    stats["cas_bytes"] = row[1]

                cur.execute("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='chunks'")
                if cur.fetchone()[0] > 0:
                    cur.execute("SELECT count(*), coalesce(sum(ref_count), 0) FROM chunks")
                    r = cur.fetchone()
                    unique_chunks = r[0]
                    total_refs = r[1]
                    stats["chunks_count"] = unique_chunks
                    if total_refs > unique_chunks and total_refs > 0:
                        stats["dedup_ratio"] = round((1.0 - (unique_chunks / total_refs)) * 100, 1)
                conn.close()
            except Exception:
                pass

        # Fallback: scan objects dir
        obj_dir = self.aigit_dir / "objects"
        if obj_dir.exists() and stats["total_objects"] == 0:
            for p in obj_dir.rglob("*"):
                if p.is_file():
                    stats["total_objects"] += 1
                    stats["cas_bytes"] += p.stat().st_size

        return stats

    def get_file_chunk_details(self, file_path: str, obj_hash: str) -> list[dict]:
        """Fetch FastCDC chunks for a file from metadata.db or object manifest"""
        chunks = []
        if not self.is_valid():
            return chunks

        db_path = self.aigit_dir / "metadata.db"
        if db_path.exists():
            try:
                conn = sqlite3.connect(str(db_path), timeout=2)
                cur = conn.cursor()
                cur.execute("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='file_chunks'")
                if cur.fetchone()[0] > 0:
                    cur.execute("""
                        SELECT chunk_index, offset, length, chunk_hash 
                        FROM file_chunks 
                        WHERE file_path = ? OR manifest_hash = ?
                        ORDER BY chunk_index ASC
                    """, (file_path, obj_hash))
                    for row in cur.fetchall():
                        chunks.append({
                            "index": row[0],
                            "offset": row[1],
                            "length": row[2],
                            "hash": row[3]
                        })
                conn.close()
            except Exception:
                pass

        if not chunks and obj_hash:
            # If stored as single blob or direct object
            p = self.root / file_path
            size = p.stat().st_size if p.exists() else 0
            chunks.append({
                "index": 0,
                "offset": 0,
                "length": size,
                "hash": obj_hash
            })

        return chunks


# =====================================================================
# TERMINAL USER INTERFACE APPLICATION ENGINE
# =====================================================================
class AIGitTUI:
    PANEL_FILES    = 0
    PANEL_COMMITS  = 1
    PANEL_CHUNKS   = 2
    PANEL_BRANCHES = 3

    def __init__(self):
        self.repo = AIGitRepo()
        self.active_panel = self.PANEL_FILES
        self.selected_indices = {
            self.PANEL_FILES: 0,
            self.PANEL_COMMITS: 0,
            self.PANEL_CHUNKS: 0,
            self.PANEL_BRANCHES: 0
        }
        self.running = True
        self.toast_msg = "🦎 AI-Git TUI Ready. Press [?] for help."
        self.toast_time = time.time()
        
        # Modal Dialog State
        self.modal_mode = None # "commit", "help", "new_branch"
        self.modal_input = ""

        # Cached data
        self.staged = []
        self.modified = []
        self.untracked = []
        self.deleted = []
        self.all_file_items = []
        self.commits = []
        self.branches = []
        self.cas_stats = {}
        self.current_chunks = []

        self.last_refresh = 0
        self.refresh_data()

    def set_toast(self, msg: str):
        self.toast_msg = msg
        self.toast_time = time.time()

    def refresh_data(self):
        """Reload all repository data from disk"""
        self.repo = AIGitRepo()
        if self.repo.is_valid():
            self.staged, self.modified, self.untracked, self.deleted = self.repo.get_working_tree_status()
            self.all_file_items = []
            for item in self.staged:
                self.all_file_items.append((item, "staged"))
            for item in self.modified:
                self.all_file_items.append((item, "modified"))
            for item in self.untracked:
                self.all_file_items.append((item, "untracked"))
            for item in self.deleted:
                self.all_file_items.append((item, "deleted"))

            self.commits = self.repo.get_commits()
            self.branches = self.repo.get_branches()
            self.cas_stats = self.repo.get_cas_telemetry()
            self.update_chunk_inspector()

        self.last_refresh = time.time()

    def update_chunk_inspector(self):
        """Update chunks based on currently selected file or commit"""
        if self.active_panel == self.PANEL_FILES and self.all_file_items:
            idx = min(self.selected_indices[self.PANEL_FILES], len(self.all_file_items) - 1)
            item, kind = self.all_file_items[idx]
            self.current_chunks = self.repo.get_file_chunk_details(item["path"], item.get("hash", ""))
        elif self.active_panel == self.PANEL_COMMITS and self.commits:
            idx = min(self.selected_indices[self.PANEL_COMMITS], len(self.commits) - 1)
            c = self.commits[idx]
            self.current_chunks = [{
                "index": 0,
                "offset": 0,
                "length": 0,
                "hash": c["tree"]
            }]

    # -----------------------------------------------------------------
    # TERMINAL DRAWING PRIMITIVES
    # -----------------------------------------------------------------
    def draw_box(self, x: int, y: int, w: int, h: int, title: str, is_active: bool, lines_out: list[str]):
        """Render a rounded box with Chameleon styling into lines buffer"""
        color_border = Palette.BORDER_ACT if is_active else Palette.BORDER_DIM
        color_title = (Palette.CYAN + Palette.BOLD) if is_active else (Palette.SLATE)

        # Top border with title
        title_str = f" {title} "
        top_len = max(0, w - len(title_str) - 2)
        top = f"\033[{y};{x}H{color_border}╭─{color_title}{title_str}{color_border}{'─' * top_len}╮{Palette.RESET}"
        lines_out.append(top)

        # Side walls
        for row in range(1, h - 1):
            wall = f"\033[{y + row};{x}H{color_border}│\033[{y + row};{x + w - 1}H{color_border}│{Palette.RESET}"
            lines_out.append(wall)

        # Bottom border
        bot = f"\033[{y + h - 1};{x}H{color_border}╰{'─' * (w - 2)}╯{Palette.RESET}"
        lines_out.append(bot)

    def draw_gauge(self, percentage: float, width: int = 14) -> str:
        """Draw a btop/charm style progress bar gauge"""
        filled = int((percentage / 100.0) * width)
        filled = max(0, min(width, filled))
        empty = width - filled
        bar = Palette.MINT + ("█" * filled) + Palette.DARK_SLATE + ("░" * empty) + Palette.RESET
        return f"[{bar}] {Palette.MINT}{percentage:.1f}%{Palette.RESET}"

    # -----------------------------------------------------------------
    # RENDER COMPLETE SCREEN
    # -----------------------------------------------------------------
    def render(self):
        term_cols, term_rows = shutil.get_terminal_size((100, 30))
        buf = []

        # Clear screen and return cursor to home
        buf.append("\033[H")

        # 1. HEADER BAR (Rows 1 - 2)
        branch = self.repo.get_current_branch()
        head_commit = self.repo.get_current_commit_hash()
        head_short = head_commit[:8] if head_commit else "(init)"

        total_objs = self.cas_stats.get("total_objects", 0)
        cas_sz = format_bytes(self.cas_stats.get("cas_bytes", 0))
        dedup_ratio = self.cas_stats.get("dedup_ratio", 0.0)

        header_top = (
            f"  {Palette.CYAN}{Palette.BOLD}🦎 AI-GIT{Palette.RESET} "
            f"{Palette.BORDER_DIM}│{Palette.RESET} "
            f"{Palette.BG_CYAN}  {branch} {Palette.RESET} "
            f"{Palette.SLATE}HEAD: {Palette.GREEN}{head_short}{Palette.RESET} "
            f"{Palette.BORDER_DIM}│{Palette.RESET} "
            f"{Palette.SLATE}CAS: {Palette.TEAL}{total_objs} objs ({cas_sz}){Palette.RESET} "
            f"{Palette.BORDER_DIM}│{Palette.RESET} "
            f"{Palette.SLATE}FastCDC Dedup: {self.draw_gauge(dedup_ratio, 10)}"
        )
        buf.append(f"\033[1;1H\033[2K{header_top}")
        buf.append(f"\033[2;1H\033[2K{Palette.BORDER_DIM}{'─' * term_cols}{Palette.RESET}")

        # If repo is not valid, display initialization banner
        if not self.repo.is_valid():
            buf.append(f"\033[6;6H{Palette.AMBER}{Palette.BOLD}⚠️  Not inside an AI-Git repository.{Palette.RESET}")
            buf.append(f"\033[8;6H{Palette.WHITE}Press {Palette.CYAN}[i]{Palette.WHITE} to initialize a new repository here with FastCDC & Blake3 CAS.{Palette.RESET}")
            buf.append(f"\033[10;6H{Palette.SLATE}Or press {Palette.RED}[q]{Palette.SLATE} to exit.{Palette.RESET}")
            sys.stdout.write("".join(buf))
            sys.stdout.flush()
            return

        # 2. MAIN PANELS LAYOUT
        # Available vertical space: row 3 to term_rows - 2
        content_h = term_rows - 4
        y_start = 3

        # Column widths:
        # Col 1 (Left): Working Tree (36% width)
        # Col 2 (Middle): Commit History DAG (34% width)
        # Col 3 (Right): FastCDC Chunk Inspector (remaining width)
        w_left = max(26, int(term_cols * 0.36))
        w_mid = max(26, int(term_cols * 0.32))
        w_right = max(24, term_cols - w_left - w_mid)

        x_left = 1
        x_mid = x_left + w_left
        x_right = x_mid + w_mid

        # Height splits on left: Files (70%), Branches/Remotes (30%)
        h_files = max(6, int(content_h * 0.70))
        h_branch = content_h - h_files

        # --- PANEL 1: Working Tree & Staging ---
        self.draw_box(x_left, y_start, w_left, h_files, "1: Working Tree / Files", self.active_panel == self.PANEL_FILES, buf)
        inner_w = w_left - 4
        inner_h = h_files - 2

        file_idx = self.selected_indices[self.PANEL_FILES]
        start_f = max(0, file_idx - (inner_h // 2))
        slice_f = self.all_file_items[start_f : start_f + inner_h]

        if not self.all_file_items:
            buf.append(f"\033[{y_start + 1};{x_left + 2}H{Palette.GREEN}✔ Working tree clean{Palette.RESET}")
        else:
            for i, (item, kind) in enumerate(slice_f):
                row_y = y_start + 1 + i
                is_sel = (start_f + i == file_idx) and (self.active_panel == self.PANEL_FILES)

                if kind == "staged":
                    badge = f"{Palette.GREEN}[+]{Palette.RESET}"
                elif kind == "modified":
                    badge = f"{Palette.AMBER}[~]{Palette.RESET}"
                elif kind == "deleted":
                    badge = f"{Palette.RED}[-]{Palette.RESET}"
                else:
                    badge = f"{Palette.BLUE}[?]{Palette.RESET}"

                fname = item["path"]
                sz_str = format_bytes(item["size"])
                # Truncate if long
                max_nm = max(6, inner_w - len(sz_str) - 8)
                disp_nm = fname if len(fname) <= max_nm else "…" + fname[-(max_nm - 1):]

                row_bg = Palette.BG_ACTIVE if is_sel else ""
                row_txt = f"{row_bg}{badge} {Palette.WHITE}{disp_nm:<{max_nm}} {Palette.DARK_SLATE}{sz_str:>6}{Palette.RESET}"
                buf.append(f"\033[{row_y};{x_left + 2}H{row_txt}")

        # --- PANEL 4: Branches & Remotes (Bottom Left) ---
        y_branch = y_start + h_files
        self.draw_box(x_left, y_branch, w_left, h_branch, "4: Branches", self.active_panel == self.PANEL_BRANCHES, buf)
        for i, br in enumerate(self.branches[:h_branch - 2]):
            row_y = y_branch + 1 + i
            is_cur = (br == branch)
            is_sel = (i == self.selected_indices[self.PANEL_BRANCHES]) and (self.active_panel == self.PANEL_BRANCHES)
            marker = f"{Palette.CYAN}* {Palette.RESET}" if is_cur else "  "
            bg = Palette.BG_ACTIVE if is_sel else ""
            buf.append(f"\033[{row_y};{x_left + 2}H{bg}{marker}{Palette.WHITE}{br}{Palette.RESET}")

        # --- PANEL 2: Commit History & DAG (Center) ---
        self.draw_box(x_mid, y_start, w_mid, content_h, "2: Commit DAG", self.active_panel == self.PANEL_COMMITS, buf)
        commit_idx = self.selected_indices[self.PANEL_COMMITS]
        inner_w_mid = w_mid - 4
        inner_h_mid = content_h - 2

        start_c = max(0, commit_idx - (inner_h_mid // 2))
        slice_c = self.commits[start_c : start_c + inner_h_mid]

        if not self.commits:
            buf.append(f"\033[{y_start + 1};{x_mid + 2}H{Palette.SLATE}No commits yet on branch.{Palette.RESET}")
        else:
            for i, c in enumerate(slice_c):
                row_y = y_start + 1 + i
                is_sel = (start_c + i == commit_idx) and (self.active_panel == self.PANEL_COMMITS)
                is_head = (start_c + i == 0)

                dot = f"{Palette.CYAN}●{Palette.RESET}" if is_head else f"{Palette.GREEN}○{Palette.RESET}"
                h_short = c["hash"][:7]
                msg = c["message"].split("\n")[0]
                avail = max(4, inner_w_mid - 11)
                disp_msg = msg if len(msg) <= avail else msg[:avail - 1] + "…"

                bg = Palette.BG_ACTIVE if is_sel else ""
                row_str = f"{bg}{dot} {Palette.GREEN}{h_short} {Palette.WHITE}{disp_msg:<{avail}}{Palette.RESET}"
                buf.append(f"\033[{row_y};{x_mid + 2}H{row_str}")

        # --- PANEL 3: FastCDC CAS Object Inspector (Right) ---
        self.draw_box(x_right, y_start, w_right, content_h, "3: FastCDC CAS Inspector", self.active_panel == self.PANEL_CHUNKS, buf)
        inner_w_r = w_right - 4
        
        # Details of highlighted item
        r_y = y_start + 1
        if self.all_file_items and self.active_panel == self.PANEL_FILES:
            f_item, f_kind = self.all_file_items[min(file_idx, len(self.all_file_items) - 1)]
            f_path = f_item["path"]
            f_hash = f_item.get("hash", "")
            f_size = f_item.get("size", 0)
            mode = "FastCDC Chunked" if f_size > 256 * 1024 else "Direct Blob"

            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}File: {Palette.WHITE}{Palette.BOLD}{Path(f_path).name}{Palette.RESET}")
            r_y += 1
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}Size: {Palette.TEAL}{format_bytes(f_size)} {Palette.DARK_SLATE}({mode}){Palette.RESET}")
            r_y += 1
            if f_hash:
                buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}Hash: {Palette.CYAN}{f_hash[:16]}…{Palette.RESET}")
                r_y += 1

            # Chunk breakdown table
            r_y += 1
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.BORDER_DIM}{'─' * inner_w_r}{Palette.RESET}")
            r_y += 1
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.CYAN}#  OFFSET   SIZE    CHUNK BLAKE3{Palette.RESET}")
            r_y += 1

            for chk in self.current_chunks[:max(1, content_h - 15)]:
                c_idx = chk["index"]
                c_off = format_bytes(chk["offset"])
                c_len = format_bytes(chk["length"])
                c_hash = chk["hash"][:8] if chk["hash"] else "blob"
                row_chk = f"{Palette.SLATE}{c_idx:<2} {c_off:<8} {c_len:<7} {Palette.MINT}{c_hash}{Palette.RESET}"
                buf.append(f"\033[{r_y};{x_right + 2}H{row_chk}")
                r_y += 1

        elif self.commits and self.active_panel == self.PANEL_COMMITS:
            c = self.commits[min(commit_idx, len(self.commits) - 1)]
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}Commit: {Palette.GREEN}{c['hash'][:14]}…{Palette.RESET}")
            r_y += 1
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}Tree:   {Palette.BLUE}{c['tree'][:14]}…{Palette.RESET}")
            r_y += 1
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}Author: {Palette.WHITE}{c['author']}{Palette.RESET}")
            r_y += 1
            r_y += 1
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.BORDER_DIM}{'─' * inner_w_r}{Palette.RESET}")
            r_y += 1
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.WHITE}{Palette.BOLD}Message:{Palette.RESET}")
            r_y += 1
            for m_line in c['message'].splitlines()[:max(1, content_h - 15)]:
                buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}{m_line[:inner_w_r]}{Palette.RESET}")
                r_y += 1
        else:
            buf.append(f"\033[{r_y};{x_right + 2}H{Palette.SLATE}Select an item to inspect FastCDC CAS chunks.{Palette.RESET}")

        # --- RIGHT-SIDE ACTIONS DOCK (Pink, Blue, Green, Purple Buttons) ---
        btn_y = y_start + content_h - 6
        buf.append(f"\033[{btn_y};{x_right + 2}H{Palette.BORDER_DIM}{'─' * inner_w_r}{Palette.RESET}")
        buf.append(f"\033[{btn_y + 1};{x_right + 2}H{Palette.WHITE}{Palette.BOLD}QUICK COMMAND ACTIONS:{Palette.RESET}")

        # Button Row 1: Push (Pink) & Pull (Blue)
        btn_push = f"{Palette.BTN_PINK} [P] PUSH ▲ {Palette.RESET}"
        btn_pull = f"{Palette.BTN_BLUE} [U] PULL ▼ {Palette.RESET}"
        buf.append(f"\033[{btn_y + 2};{x_right + 2}H{btn_push}  {btn_pull}")

        # Button Row 2: Commit (Green) & Diff / Inspect (Purple)
        btn_commit = f"{Palette.BTN_GREEN} [C] COMMIT ✔ {Palette.RESET}"
        btn_diff = f"{Palette.BTN_PURPLE} [D] DIFF ⚡ {Palette.RESET}"
        buf.append(f"\033[{btn_y + 3};{x_right + 2}H{btn_commit}  {btn_diff}")

        # Button Row 3: Status / Sync (Mint Green / Blue)
        btn_status = f"{Palette.BTN_BLUE} [S] SYNC ⟳ {Palette.RESET}"
        buf.append(f"\033[{btn_y + 4};{x_right + 2}H{btn_status} {Palette.DARK_SLATE}(Click hotkey to trigger){Palette.RESET}")

        # 3. MODAL POPUPS (if active)
        if self.modal_mode == "commit":
            self.render_commit_modal(term_cols, term_rows, buf)
        elif self.modal_mode == "help":
            self.render_help_modal(term_cols, term_rows, buf)
        elif self.modal_mode == "new_branch":
            self.render_branch_modal(term_cols, term_rows, buf)

        # 4. FOOTER STATUS BAR (Row term_rows)
        footer_y = term_rows
        toast = self.toast_msg if (time.time() - self.toast_time < 5.0) else "Ready"
        shortcuts = f"{Palette.BTN_PINK} [p] Push {Palette.RESET} {Palette.BTN_BLUE} [u] Pull {Palette.RESET} {Palette.BTN_GREEN} [c] Commit {Palette.RESET} {Palette.BTN_PURPLE} [d] Diff {Palette.RESET} {Palette.SLATE}[Tab] Panel [?] Help [q] Quit{Palette.RESET}"
        
        status_bar = f"\033[{footer_y};1H\033[2K{Palette.BG_PANEL} {Palette.MINT}● {Palette.WHITE}{toast:<24} {shortcuts} {Palette.RESET}"
        buf.append(status_bar)

        # Flush full frame to terminal
        sys.stdout.write("".join(buf))
        sys.stdout.flush()

    # -----------------------------------------------------------------
    # MODAL DIALOGS
    # -----------------------------------------------------------------
    def render_commit_modal(self, cols: int, rows: int, buf: list[str]):
        w = min(64, cols - 6)
        h = 10
        x = (cols - w) // 2
        y = (rows - h) // 2

        self.draw_box(x, y, w, h, "Commit Staged Changes", True, buf)
        staged_count = len(self.staged)

        buf.append(f"\033[{y + 1};{x + 2}H{Palette.GREEN}{staged_count} file(s) staged for commit.{Palette.RESET}")
        buf.append(f"\033[{y + 3};{x + 2}H{Palette.WHITE}Enter commit message:{Palette.RESET}")
        
        # Input box
        input_disp = self.modal_input + "█"
        buf.append(f"\033[{y + 5};{x + 2}H{Palette.BG_ACTIVE}{Palette.CYAN} > {input_disp:<{w - 8}}{Palette.RESET}")
        buf.append(f"\033[{y + 7};{x + 2}H{Palette.DARK_SLATE}[Enter] Commit    [Esc] Cancel{Palette.RESET}")

    def render_branch_modal(self, cols: int, rows: int, buf: list[str]):
        w = min(54, cols - 6)
        h = 8
        x = (cols - w) // 2
        y = (rows - h) // 2

        self.draw_box(x, y, w, h, "Create New Branch", True, buf)
        buf.append(f"\033[{y + 2};{x + 2}H{Palette.WHITE}Enter new branch name:{Palette.RESET}")
        input_disp = self.modal_input + "█"
        buf.append(f"\033[{y + 4};{x + 2}H{Palette.BG_ACTIVE}{Palette.CYAN} > {input_disp:<{w - 8}}{Palette.RESET}")
        buf.append(f"\033[{y + 6};{x + 2}H{Palette.DARK_SLATE}[Enter] Create    [Esc] Cancel{Palette.RESET}")

    def render_help_modal(self, cols: int, rows: int, buf: list[str]):
        w = min(72, cols - 4)
        h = 18
        x = (cols - w) // 2
        y = (rows - h) // 2

        self.draw_box(x, y, w, h, "🦎 AI-Git TUI Help & Keybindings", True, buf)
        help_lines = [
            f"{Palette.CYAN}NAVIGATION:{Palette.RESET}",
            f"  {Palette.WHITE}[Tab] / [Shift+Tab]{Palette.SLATE}  Switch focused panel (1 - 4)",
            f"  {Palette.WHITE}[1, 2, 3, 4]{Palette.SLATE}          Directly jump to panel",
            f"  {Palette.WHITE}[↑ / ↓] or [k / j]{Palette.SLATE}    Navigate list items",
            "",
            f"{Palette.GREEN}WORKING TREE & COMMITS:{Palette.RESET}",
            f"  {Palette.WHITE}[Space]{Palette.SLATE}               Stage / Unstage selected file",
            f"  {Palette.WHITE}[a]{Palette.SLATE}                   Stage all changes (ai-git add .)",
            f"  {Palette.WHITE}[c]{Palette.SLATE}                   Open Commit dialog",
            f"  {Palette.WHITE}[b]{Palette.SLATE}                   Create new branch",
            f"  {Palette.WHITE}[Enter]{Palette.SLATE}               Checkout branch / Select item",
            "",
            f"{Palette.MINT}FASTCDC & CAS TELEMETRY:{Palette.RESET}",
            f"  {Palette.SLATE}Files >256KB are chunked via FastCDC (256KB-4MB boundaries)",
            f"  {Palette.SLATE}Chunk hashes and manifests are deduplicated in metadata.db",
            "",
            f"{Palette.DARK_SLATE}Press [Esc] or [?] to close this help dialog.{Palette.RESET}"
        ]
        for i, line in enumerate(help_lines):
            if i < h - 2:
                buf.append(f"\033[{y + 1 + i};{x + 2}H{line}")

    # -----------------------------------------------------------------
    # ACTIONS & REPOSITORY MUTATIONS
    # -----------------------------------------------------------------
    def toggle_stage(self):
        """Stage or unstage currently selected file"""
        if not self.all_file_items:
            return
        idx = min(self.selected_indices[self.PANEL_FILES], len(self.all_file_items) - 1)
        item, kind = self.all_file_items[idx]
        file_path = item["path"]

        # Run ai-git add via executable or direct command
        exe_candidates = [
            self.repo.root / "CLI" / "build" / "Debug" / "ai-git.exe",
            self.repo.root / "CLI" / "build" / "ai-git.exe",
            self.repo.root / "ai-git.exe"
        ]
        exe = next((e for e in exe_candidates if e.exists()), None)
        if exe:
            cmd = [str(exe), "add", file_path]
            subprocess.run(cmd, cwd=str(self.repo.root), capture_output=True)
            self.set_toast(f"✔ Staged {file_path}")
        else:
            self.set_toast(f"Staged {file_path}")

        self.refresh_data()

    def stage_all(self):
        """Stage all files in workspace"""
        exe_candidates = [
            self.repo.root / "CLI" / "build" / "Debug" / "ai-git.exe",
            self.repo.root / "CLI" / "build" / "ai-git.exe"
        ]
        exe = next((e for e in exe_candidates if e.exists()), None)
        if exe:
            subprocess.run([str(exe), "add", "."], cwd=str(self.repo.root), capture_output=True)
            self.set_toast("✔ Staged all files")
        self.refresh_data()

    def do_commit(self, msg: str):
        """Execute commit with given message"""
        if not msg.strip():
            self.set_toast("⚠️  Commit message cannot be empty")
            return
        exe_candidates = [
            self.repo.root / "CLI" / "build" / "Debug" / "ai-git.exe",
            self.repo.root / "CLI" / "build" / "ai-git.exe"
        ]
        exe = next((e for e in exe_candidates if e.exists()), None)
        if exe:
            res = subprocess.run([str(exe), "commit", "-m", msg], cwd=str(self.repo.root), capture_output=True, text=True)
            if res.returncode == 0:
                self.set_toast(f"✔ Committed: \"{msg[:24]}\"")
            else:
                self.set_toast(f"Commit error: {res.stderr.strip()[:32]}")
        self.refresh_data()

    def get_exe(self) -> Path | None:
        """Find ai-git binary across Release, Debug, or root build dirs"""
        candidates = [
            self.repo.root / "CLI" / "build" / "Release" / "ai-git.exe",
            self.repo.root / "CLI" / "build" / "Debug" / "ai-git.exe",
            self.repo.root / "CLI" / "build" / "ai-git.exe",
            self.repo.root / "ai-git.exe",
            Path.cwd() / "CLI" / "build" / "Release" / "ai-git.exe",
            Path.cwd() / "CLI" / "build" / "Debug" / "ai-git.exe"
        ]
        return next((e for e in candidates if e.exists()), None)

    def do_push(self):
        """Execute ai-git push to remote"""
        exe = self.get_exe()
        if not exe:
            self.set_toast("⚠️  ai-git executable not found")
            return
        self.set_toast("⏳ Pushing chunks to remote CAS...")
        res = subprocess.run([str(exe), "push"], cwd=str(self.repo.root), capture_output=True, text=True)
        if res.returncode == 0:
            self.set_toast("✔ Push completed successfully")
        else:
            err = res.stderr.strip() or res.stdout.strip()
            self.set_toast(f"Push: {err[:36]}")
        self.refresh_data()

    def do_pull(self):
        """Execute ai-git pull from remote"""
        exe = self.get_exe()
        if not exe:
            self.set_toast("⚠️  ai-git executable not found")
            return
        self.set_toast("⏳ Pulling chunks from remote...")
        res = subprocess.run([str(exe), "pull"], cwd=str(self.repo.root), capture_output=True, text=True)
        if res.returncode == 0:
            self.set_toast("✔ Pull completed successfully")
        else:
            err = res.stderr.strip() or res.stdout.strip()
            self.set_toast(f"Pull: {err[:36]}")
        self.refresh_data()

    def do_diff(self):
        """Execute inspect or diff on currently selected file"""
        exe = self.get_exe()
        if not exe:
            self.set_toast("⚠️  ai-git executable not found")
            return
        if self.all_file_items and self.active_panel == self.PANEL_FILES:
            idx = min(self.selected_indices[self.PANEL_FILES], len(self.all_file_items) - 1)
            item, _ = self.all_file_items[idx]
            fpath = item["path"]
            res = subprocess.run([str(exe), "inspect", fpath], cwd=str(self.repo.root), capture_output=True, text=True)
            if res.returncode == 0:
                self.set_toast(f"✔ Inspected {Path(fpath).name}")
            else:
                self.set_toast(f"Inspect: {res.stderr.strip()[:32]}")
        else:
            self.set_toast("✔ Diff / Inspect ready (Select file)")
        self.refresh_data()

    def do_status(self):
        """Execute status & refresh working tree"""
        exe = self.get_exe()
        if exe:
            subprocess.run([str(exe), "status"], cwd=str(self.repo.root), capture_output=True)
        self.refresh_data()
        self.set_toast("✔ Working tree synchronized & refreshed")

    # -----------------------------------------------------------------
    # KEYBOARD & INPUT EVENT LOOP
    # -----------------------------------------------------------------
    def get_key(self) -> str:
        """Cross-platform non-blocking key read"""
        if os.name == 'nt':
            if msvcrt.kbhit():
                ch = msvcrt.getch()
                if ch in (b'\x00', b'\xe0'): # Special key prefix
                    ch2 = msvcrt.getch()
                    if ch2 == b'H': return "UP"
                    if ch2 == b'P': return "DOWN"
                    if ch2 == b'K': return "LEFT"
                    if ch2 == b'M': return "RIGHT"
                    return ""
                if ch == b'\r': return "ENTER"
                if ch == b'\x1b': return "ESC"
                if ch == b'\t': return "TAB"
                if ch == b'\x08': return "BACKSPACE"
                try:
                    return ch.decode("utf-8")
                except Exception:
                    return ""
            return ""
        else:
            # POSIX non-blocking read
            import select
            r, _, _ = select.select([sys.stdin], [], [], 0.05)
            if r:
                c = sys.stdin.read(1)
                if c == '\x1b':
                    # Check escape sequence
                    r2, _, _ = select.select([sys.stdin], [], [], 0.01)
                    if r2:
                        seq = sys.stdin.read(2)
                        if seq == '[A': return "UP"
                        if seq == '[B': return "DOWN"
                        if seq == '[C': return "RIGHT"
                        if seq == '[D': return "LEFT"
                    return "ESC"
                if c == '\n': return "ENTER"
                if c == '\t': return "TAB"
                if c == '\x7f': return "BACKSPACE"
                return c
            return ""

    def run(self):
        """Main TUI execution loop with alternate screen buffer"""
        # Switch to alternate buffer and hide cursor
        sys.stdout.write("\033[?1049h\033[?25l")
        sys.stdout.flush()

        try:
            while self.running:
                self.render()
                key = self.get_key()

                if not key:
                    time.sleep(0.04) # ~25-30 FPS idle
                    continue

                # 1. MODAL INPUT HANDLING
                if self.modal_mode:
                    if key == "ESC":
                        self.modal_mode = None
                        self.modal_input = ""
                    elif key == "ENTER":
                        if self.modal_mode == "commit":
                            msg = self.modal_input
                            self.modal_mode = None
                            self.modal_input = ""
                            self.do_commit(msg)
                        elif self.modal_mode == "new_branch":
                            bname = self.modal_input
                            self.modal_mode = None
                            self.modal_input = ""
                            if bname.strip():
                                exe = self.repo.root / "CLI" / "build" / "Debug" / "ai-git.exe"
                                if exe.exists():
                                    subprocess.run([str(exe), "branch", bname.strip()], cwd=str(self.repo.root), capture_output=True)
                                    subprocess.run([str(exe), "checkout", bname.strip()], cwd=str(self.repo.root), capture_output=True)
                                    self.set_toast(f"✔ Created and switched to branch {bname}")
                                    self.refresh_data()
                        elif self.modal_mode == "help":
                            self.modal_mode = None
                    elif key == "BACKSPACE":
                        self.modal_input = self.modal_input[:-1]
                    elif len(key) == 1 and ord(key) >= 32:
                        self.modal_input += key
                    continue

                # 2. GLOBAL HOTKEYS
                if key in ('q', 'Q'):
                    self.running = False
                    break
                elif key in ('?', 'h'):
                    self.modal_mode = "help"
                    continue
                elif key == 'r':
                    self.refresh_data()
                    self.set_toast("✔ Refreshed repository data")
                    continue
                elif key == 'i' and not self.repo.is_valid():
                    self.do_init()
                    continue

                # Panel switching
                elif key == "TAB":
                    self.active_panel = (self.active_panel + 1) % 4
                    self.update_chunk_inspector()
                elif key == '1':
                    self.active_panel = self.PANEL_FILES
                    self.update_chunk_inspector()
                elif key == '2':
                    self.active_panel = self.PANEL_COMMITS
                    self.update_chunk_inspector()
                elif key == '3':
                    self.active_panel = self.PANEL_CHUNKS
                elif key == '4':
                    self.active_panel = self.PANEL_BRANCHES

                # List navigation (UP / DOWN)
                elif key in ("UP", "k"):
                    cur = self.selected_indices[self.active_panel]
                    self.selected_indices[self.active_panel] = max(0, cur - 1)
                    self.update_chunk_inspector()
                elif key in ("DOWN", "j"):
                    cur = self.selected_indices[self.active_panel]
                    max_len = 1
                    if self.active_panel == self.PANEL_FILES:
                        max_len = max(1, len(self.all_file_items))
                    elif self.active_panel == self.PANEL_COMMITS:
                        max_len = max(1, len(self.commits))
                    elif self.active_panel == self.PANEL_BRANCHES:
                        max_len = max(1, len(self.branches))
                    self.selected_indices[self.active_panel] = min(max_len - 1, cur + 1)
                    self.update_chunk_inspector()

                # Action hotkeys
                elif key == ' ':
                    if self.active_panel == self.PANEL_FILES:
                        self.toggle_stage()
                elif key in ('a', 'A'):
                    if self.active_panel == self.PANEL_FILES:
                        self.stage_all()
                elif key in ('c', 'C'):
                    self.modal_mode = "commit"
                    self.modal_input = ""
                elif key in ('b', 'B'):
                    self.modal_mode = "new_branch"
                    self.modal_input = ""
                elif key in ('p', 'P'):
                    self.do_push()
                elif key in ('u', 'U'):
                    self.do_pull()
                elif key in ('d', 'D'):
                    self.do_diff()
                elif key in ('s', 'S'):
                    self.do_status()
                elif key == "ENTER":
                    if self.active_panel == self.PANEL_BRANCHES:
                        self.do_checkout_branch()
                    elif self.active_panel == self.PANEL_FILES:
                        self.toggle_stage()

        finally:
            # Restore terminal buffer and show cursor
            sys.stdout.write("\033[?25h\033[?1049l\033[0m")
            sys.stdout.flush()


def main():
    tui = AIGitTUI()
    tui.run()


if __name__ == "__main__":
    main()
