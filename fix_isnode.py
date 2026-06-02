import re

with open('/Users/thaongan/Downloads/CarLine/CarLine.ino', 'r') as f:
    text = f.read()

orig_isnode = """bool isNode(){
  return digitalRead(S2)==1 &&
         digitalRead(S3)==1 &&
         digitalRead(S4)==1;
}"""

new_isnode = """bool isNode(){
  // Chống lỗi đếm Node trên đường thẳng (do băng dính dày làm xe lắc lên 3 mắt giữa)
  // Tính tổng số mắt đang đè lên vạch đen
  int sum = digitalRead(S1) + digitalRead(S2) + digitalRead(S3) + digitalRead(S4) + digitalRead(S5);
  
  // Mắt rìa ngoài cùng (S1 hoặc S5) phải thấy vạch ngang, và mắt giữa S3 cũng phải thấy
  bool hasOuter = (digitalRead(S1) == 1 || digitalRead(S5) == 1);
  bool hasCenter = (digitalRead(S3) == 1);

  // Điều kiện để là 1 ngã tư chuẩn:
  // 1. Quét trúng ít nhất 4 mắt cùng lúc (vạch ngang rộng)
  // 2. HOẶC mắt ngoài cùng và mắt trong cùng đều chạm => Chắc chắn là vạch ngang ngã tư chạy cắt qua!
  return (sum >= 4) || (hasOuter && hasCenter);
}"""

text = text.replace(orig_isnode, new_isnode)

with open('/Users/thaongan/Downloads/CarLine/CarLine.ino', 'w') as f:
    f.write(text)

print("FIXED ISNODE SENSOR")
