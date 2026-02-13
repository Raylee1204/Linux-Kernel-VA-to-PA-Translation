# Linux 系統呼叫實作：虛擬位址轉實體位址 (Virtual to Physical Address Translation)

本專案在 x86-64 環境下實作了一個自定義的 Linux 系統呼叫 (System Call)，用於將 **虛擬位址 (Virtual Address, VA)** 轉換為對應的 **實體位址 (Physical Address, PA)**。實作原理涉及遍歷 Linux 核心的多層分頁表 (Multi-level Page Tables)。

為了驗證此系統呼叫的正確性，我們設計了兩個測試情境來觀察 Linux 記憶體管理的行為：
1.  **寫入時複製 (Copy-on-Write, CoW)**
2.  **需求分頁 (Demand Paging)**

## 🎯 專案目標
* 實作一個系統呼叫 (Syscall Number: 450)，接收虛擬位址作為輸入，並回傳對應的實體位址。
* 透過 CoW 與 Demand Paging 測試程式，驗證位址轉換的正確性與觀察 OS 行為。

## 💻 系統環境
* **作業系統:** Ubuntu 22.04.5 LTS (Desktop AMD64)
* **核心版本:** 5.15.137
* **虛擬機:** VMware
* **記憶體:** 4GB

---

## 📚 技術背景

### 1. Linux 分頁模型 (4-Level Paging)
在 x86-64 架構上，Linux 通常使用 4 層分頁階層 (或是較新硬體的 5 層)。控制暫存器 **CR3** (PDBR) 儲存了頂層目錄 (PGD) 的實體位址。

階層結構如下：
1.  **PGD** (Page Global Directory)
2.  **PUD** (Page Upper Directory)
3.  **PMD** (Page Middle Directory)
4.  **PTE** (Page Table Entry) -> 包含頁框號碼 (PFN)

要將邏輯位址轉換為實體位址，核心會依序查詢這些表格：
`pgd_t` -> `pud_t` -> `pmd_t` -> `pte_t` -> 取得實體位址。

<img width="685" height="483" alt="image" src="https://github.com/user-attachments/assets/fd7e7fb8-3512-4c64-949a-e55417982773" />

*(圖：Linux 分頁模型遍歷流程)*

### 2. 分頁表項目 (PTE) 結構
實體位址是從 PTE 推導出來的。我們需要透過遮罩 (Mask) 取出 **頁框號碼 (Page Frame Number, PFN)**，並將其與虛擬位址的偏移量 (Offset) 結合。

* **PTE_PFN_MASK**: 用於取出 PFN (位元 12-51)。
* **PAGE_MASK**: 用於處理 Offset (最後 12 位元)。

<img width="869" height="569" alt="image" src="https://github.com/user-attachments/assets/103457f1-177f-4ec1-9ed6-e50f33d50fbb" />

*(圖：64-bit Page Table Entry 結構)*

### 3. 關鍵核心函式
* `copy_from_user`: 安全地將資料從使用者空間複製到核心空間。
* `copy_to_user`: 安全地將資料從核心空間複製回使用者空間。
* `pgd_offset`, `pud_offset`, `pmd_offset`, `pte_offset_kernel`: 用於遍歷分頁表的巨集/函式。

---

## 🛠️ 實作細節

### 1. 核心空間程式碼 (Kernel Space)
**檔案:** `my_get_physical_addresses.c`

核心邏輯包含分頁表遍歷 (Page Table Walk)。我們取得當前行程的 `mm_struct` 並一路查詢到 PTE。

```c
/* 核心邏輯片段 */
pgd = pgd_offset(current->mm, vaddr);
// ... 錯誤檢查 ...
p4d = p4d_offset(pgd, vaddr);
pud = pud_offset(p4d, vaddr);
pmd = pmd_offset(pud, vaddr);
pte = pte_offset_kernel(pmd, vaddr);

// 計算實體位址
// 來自 PTE 的 PFN | 來自虛擬位址的 Offset
paddr = (pte_val(*pte) & PTE_PFN_MASK) | (vaddr & ~PAGE_MASK);
