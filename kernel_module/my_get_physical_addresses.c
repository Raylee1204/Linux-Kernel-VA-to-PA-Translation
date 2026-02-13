#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/mm.h>

#define PTE_PFN_MASK    ((pteval_t)PHYSICAL_PAGE_MASK)
#define PTE_FLAGS_MASK  (~PTE_PFN_MASK)

/*
 * 系統調用：獲取虛擬地址對應的物理地址
 * @user_vaddr_ptr: 用戶空間傳入的虛擬地址指針
 * @paddr_out: 用於存儲物理地址的用戶空間指針
 * 返回值：成功返回0，失敗返回負的錯誤碼
 */
SYSCALL_DEFINE2(my_get_physical_addresses, unsigned long __user *, user_vaddr_ptr, unsigned long __user *, paddr_out)
{
    unsigned long vaddr;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long paddr = 0;

    if (copy_from_user(&vaddr, user_vaddr_ptr, sizeof(vaddr)))
        return -EFAULT;

    pgd = pgd_offset(current->mm, vaddr);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return -EFAULT;

    p4d = p4d_offset(pgd, vaddr);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
        return -EFAULT;

    pud = pud_offset(p4d, vaddr);
    if (pud_none(*pud) || pud_bad(*pud))
        return -EFAULT;

    pmd = pmd_offset(pud, vaddr);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        return -EFAULT;

    pte = pte_offset_kernel(pmd, vaddr);
    if (!pte_present(*pte))
        return -EFAULT;

    paddr = (pte_val(*pte) & PTE_PFN_MASK) | (vaddr & ~PAGE_MASK);

    if (copy_to_user(paddr_out, &paddr, sizeof(paddr)))
        return -EFAULT;

    return 0;
}
