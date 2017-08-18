class Solution(object):
    def twoSum(self, nums, target):
        """
        :type nums: List[int]
        :type target: int
        :rtype: List[int]
        """
        lp = 0
        hp = len(nums)-1
        nd = [ (i, x) for i, x in enumerate(nums) ]
        nd = sorted(nd, key=lambda t: t[1])
        while lp < hp:
            s = nd[lp][1] + nd[hp][1]
            if s == target:
                return [ nd[lp][0], nd[hp][0] ]
            elif s > target:
                hp = hp - 1
            else:
                lp = lp + 1