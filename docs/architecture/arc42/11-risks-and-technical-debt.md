# 11. Risks and Technical Debt

## 11.1 Risks

- **Kernel type ambiguity** may cause design drift if not formalized soon
- **Boundary erosion** may begin before the Kernel Charter is written
- **Boot contract ambiguity** may create fragile kernel entry assumptions
- **Bootstrap contract vagueness** may blur kernel and init responsibilities
- **Fastpath overdesign** may delay real bring-up
- **Object semantics gaps** may surface later as inconsistent ownership or revocation behavior
- **Premature portability** could dilute progress before the first platform is stable

## 11.2 Deliberate Early Debt

The architecture intentionally accepts some early incompleteness:

- incomplete policy models
- incomplete device model
- incomplete deployment breadth
- incomplete performance data
- deliberately minimal early boot functionality

This is acceptable because the current target is a proven backbone, not a feature-rich release.

---
